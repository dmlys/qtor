#include <ext/utility.hpp>
#include <ext/iterator/outdirect_iterator.hpp>
#include <ext/iterator/indirect_iterator.hpp>
#include <ext/range/adaptors/outdirected.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <QtTools/ToolsBase.hpp>
#include <qtor/FileTreeModel.hqt>

namespace qtor
{
	viewed::refilter_type SimpleTextFilter::set_expr(QString expr)
	{
		expr = expr.trimmed();
		if (expr.compare(m_filterWord, Qt::CaseInsensitive) == 0)
			return viewed::refilter_type::same;

		if (expr.startsWith(m_filterWord, Qt::CaseInsensitive))
		{
			m_filterWord = expr;
			return viewed::refilter_type::incremental;
		}
		else
		{
			m_filterWord = expr;
			return viewed::refilter_type::full;
		}
	}

	bool SimpleTextFilter::matches(const QString & rec) const
	{
		return rec.contains(m_filterWord, Qt::CaseInsensitive);
	}

	SimpleTextFilter FileTreeModel::m_tfilt;
	const auto make_ref = [](auto * ptr) { return std::ref(*ptr); };
	const FileTreeModel::value_container FileTreeModel::ms_empty_container;

	inline auto FileTreeModel::get_page(const QModelIndex & index) const -> page_type *
	{
		return static_cast<page_type *>(index.internalPointer());
	}

	auto FileTreeModel::get_element_ptr(const QModelIndex & index) const -> const value_ptr &
	{
		assert(index.isValid());
		auto * page = get_page(index);
		assert(page);

		auto & seq_view = page->children.get<by_seq>();
		assert(index.row() < get_children_count(page));
		return seq_view[index.row()];
	}

	filepath_type FileTreeModel::get_name_type::operator()(const QString & filepath) const
	{
		int pos = filepath.lastIndexOf('/') + 1;
		return filepath.mid(pos);
	}

	filepath_type FileTreeModel::get_name_type::operator()(const leaf_type & leaf) const
	{
		return operator()(leaf.filename);
	}

	filepath_type FileTreeModel::get_name_type::operator()(const page_type & page) const
	{
		return operator()(page.name);
	}

	filepath_type FileTreeModel::get_name_type::operator()(const leaf_type * leaf) const
	{
		return operator()(leaf->filename);
	}

	filepath_type FileTreeModel::get_name_type::operator()(const page_type * page) const
	{
		return operator()(page->name);
	}

	filepath_type FileTreeModel::get_name_type::operator()(const value_ptr & val) const
	{
		return viewed::pv_visit(*this, val);
	}

	int FileTreeModel::columnCount(const QModelIndex & parent /* = QModelIndex() */) const
	{
		return 4;
	}

	int FileTreeModel::rowCount(const QModelIndex & parent /* = QModelIndex() */) const
	{
		if (not parent.isValid())
			return qint(get_children_count(&m_root));

		const auto & val = get_element_ptr(parent);		
		return qint(get_children_count(val));
	}

	QModelIndex FileTreeModel::parent(const QModelIndex & index) const
	{
		if (not index.isValid()) return {};
		
		page_type * page = get_page(index);
		assert(page);

		page_type * parent_page = page->parent;
		if (not parent_page) return {}; // already top level index

		auto & children = parent_page->children;
		auto & seq_view = children.get<by_seq>();

		auto code_it = children.find(page->name);
		auto seq_it = children.project<by_seq>(code_it);

		int row = qint(seq_it - seq_view.begin());
		return createIndex(row, 0, parent_page);
	}

	QModelIndex FileTreeModel::index(int row, int column, const QModelIndex & parent /* = QModelIndex() */) const
	{
		if (not parent.isValid())
		{
			return createIndex(row, column, ext::unconst(&m_root));
		}
		else
		{
			auto & element = get_element_ptr(parent);
			auto & children = get_children(element);
			auto count = get_children_count(element);

			if (count < row)
				return {};

			// only page can have children
			assert(element.index() == PAGE);
			auto * page = static_cast<page_type *>(element.pointer());
			return createIndex(row, column, page);
		}
	}

	QVariant FileTreeModel::data(const QModelIndex & index, int role /* = Qt::DisplayRole */) const
	{
		if (not index.isValid()) 
			return {};

		if (role != Qt::DisplayRole and role != Qt::ToolTipRole)
			return {};

		const auto & val = get_element_ptr(index);
		switch (index.column())
		{
			case 0:  return get_name(val);
			case 1:  return get_total_size(val);
			case 2:  return get_have_size(val);
			case 3:  return get_wanted(val);

			default: return {};
		}

	}

	QVariant FileTreeModel::headerData(int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */) const
	{
		if (orientation != Qt::Horizontal or role != Qt::DisplayRole)
			return {};
		
		switch (section)
		{
			case 0:  return QStringLiteral("fname");
			case 1:  return QStringLiteral("total size");
			case 2:  return QStringLiteral("have size");
			case 3:  return QStringLiteral("wanted");

			default: return {};
		}
	}

	auto FileTreeModel::get_model() -> model_type *
	{
		return static_cast<model_type *>(static_cast<QAbstractItemModel *>(this));
	}

	void FileTreeModel::emit_changed(int_vector::const_iterator first, int_vector::const_iterator last)
	{
		if (first == last) return;

		auto * model = get_model();
		int ncols = model->columnCount(model_type::invalid_index);

		for (; first != last; ++first)
		{
			// lower index on top, higher on bottom
			int top, bottom;
			top = bottom = *first;

			// try to find the sequences with step of 1, for example: ..., 4, 5, 6, ...
			for (++first; first != last and *first - bottom == 1; ++first, ++bottom)
				continue;

			--first;

			auto top_left = model->index(top, 0, model_type::invalid_index);
			auto bottom_right = model->index(bottom, ncols - 1, model_type::invalid_index);
			model->dataChanged(top_left, bottom_right, model_type::all_roles);
		}

	}

	void FileTreeModel::change_indexes(page_type & page, QModelIndexList::const_iterator model_index_first, QModelIndexList::const_iterator model_index_last,
									   int_vector::const_iterator first, int_vector::const_iterator last, int offset)
	{
		auto * model = get_model();
		auto size = last - first; (void)size;

		for (; model_index_first != model_index_last; ++model_index_first)
		{
			const QModelIndex & idx = *model_index_first;
			if (not idx.isValid()) continue;

			auto * pageptr = get_page(idx);
			if (pageptr != &page) continue;

			auto row = idx.row();
			auto col = idx.column();

			if (row < offset) continue;

			assert(row < size); (void)size;
			auto newIdx = createIndex(first[row - offset], col, pageptr);
			model->changePersistentIndex(idx, newIdx);
		}
	}

	void FileTreeModel::inverse_index_array(int_vector & inverse, int_vector::iterator first, int_vector::iterator last, int offset)
	{
		inverse.resize(last - first);
		int i = offset;

		for (auto it = first; it != last; ++it, ++i)
		{
			int val = *it;
			inverse[viewed::unmark_index(val) - offset] = viewed::marked_index(val) ? i : -1;
		}

		//std::copy(buffer.begin(), buffer.end(), first);
	}


	auto FileTreeModel::analyze(const QStringRef & prefix, const torrent_file & item)
		-> std::tuple<std::uintptr_t, QString, QStringRef>
	{
		const auto & path = item.filename;
		auto first = path.begin() + prefix.size();
		auto last = path.end();
		auto it = std::find(first, last, '/');

		if (it == last)
		{
			QString name = QString::null;
			return std::make_tuple(LEAF, std::move(name), prefix);
		}
		else
		{
			QString name = path.mid(prefix.size(), it - first);
			it = std::find_if_not(it, last, [](auto ch) { return ch == '/'; });
			return std::make_tuple(PAGE, std::move(name), path.leftRef(it - path.begin()));
		}
	}

	bool FileTreeModel::is_subelement(const QStringRef & prefix, const QString & name, const torrent_file & item)
	{
		auto ref = item.filename.midRef(prefix.size(), name.size());
		return ref == name;
	}

	template <class RandomAccessIterator>
	void FileTreeModel::group_by_dirs(RandomAccessIterator first, RandomAccessIterator last)
	{
		auto sort_pred = viewed::make_indirect_fun(torrent_file_id_greater());
		std::sort(first, last, sort_pred);
	}

	void FileTreeModel::reinit_view(page_type & page, reinit_context & ctx)
	{
		while (ctx.first != ctx.last)
		{
			const auto * item_ptr = *ctx.first;
			auto[type, name, newprefix] = analyze(ctx.prefix, *item_ptr);
			if (type == LEAF)
			{
				auto * leaf = item_ptr;
				page.children.insert(leaf);
				++ctx.first;
			}
			else // PAGE
			{
				auto page_ptr = std::make_unique<page_type>();
				page_ptr->parent = &page;
				page_ptr->name = name;

				auto newctx = ctx;
				newctx.prefix = std::move(newprefix);
				newctx.first = ctx.first;

				auto issub = std::bind(is_subelement, std::cref(ctx.prefix), std::cref(name), std::placeholders::_1);
				newctx.last = std::find_if_not(ctx.first, ctx.last, viewed::make_indirect_fun(std::move(issub)));
				reinit_view(*page_ptr, newctx);

				page.children.insert(std::move(page_ptr));
				ctx.first = newctx.last;
			}
		}

		page.upassed = page.children.size();
		auto & seq_view = page.children.get<by_seq>();
		auto seq_ptr_view = seq_view | ext::outdirected;

		value_ptr_vector & refs = *ctx.vptr_array;
		refs.assign(seq_ptr_view.begin(), seq_ptr_view.end());

		auto refs_first = refs.begin();
		auto refs_last = refs.end();
		stable_sort(refs_first, refs_last);

		seq_view.rearrange(boost::make_transform_iterator(refs_first, make_ref));
	}

	void FileTreeModel::reinit_view()
	{
		beginResetModel();

		m_root.upassed = 0;
		m_root.children.clear();

		value_ptr_vector valptr_array;
		reinit_context ctx;

		leaf_ptr_vector elements;
		elements.assign(
			boost::make_transform_iterator(m_owner->begin(), get_view_pointer),
			boost::make_transform_iterator(m_owner->end(), get_view_pointer)
		);

		auto first = elements.begin();
		auto last  = elements.end();
		group_by_dirs(first, last);

		ctx.vptr_array = &valptr_array;
		ctx.first = first;
		ctx.last = last;
		reinit_view(m_root, ctx);

		endResetModel();
	}

	void FileTreeModel::merge_newdata(value_ptr_iterator first, value_ptr_iterator middle, value_ptr_iterator last, bool resort_old /* = true */)
	{
		if (not viewed::active(m_sorter)) return;

		auto sorter = value_ptr_sorter_type(m_sorter);
		auto comp = viewed::make_indirect_fun(std::move(sorter));

		if (resort_old) varalgo::stable_sort(first, middle, comp);
		varalgo::sort(middle, last, comp);
		varalgo::inplace_merge(first, middle, last, comp);
	}

	void FileTreeModel::merge_newdata(value_ptr_iterator first, value_ptr_iterator middle, value_ptr_iterator last,
	                                  int_vector::iterator ifirst, int_vector::iterator imiddle, int_vector::iterator ilast, bool resort_old /* = true */)
	{
		if (not viewed::active(m_sorter)) return;

		assert(last - first == ilast - ifirst);
		assert(middle - first == imiddle - ifirst);

		auto sorter = value_ptr_sorter_type(m_sorter);
		auto comp = viewed::make_get_functor<0>(viewed::make_indirect_fun(std::move(sorter)));

		auto zfirst  = ext::make_zip_iterator(first, ifirst);
		auto zmiddle = ext::make_zip_iterator(middle, imiddle);
		auto zlast   = ext::make_zip_iterator(last, ilast);

		if (resort_old) varalgo::stable_sort(zfirst, zmiddle, comp);
		varalgo::sort(zmiddle, zlast, comp);
		varalgo::inplace_merge(zfirst, zmiddle, zlast, comp);
	}

	void FileTreeModel::stable_sort(value_ptr_iterator first, value_ptr_iterator last)
	{
		if (not viewed::active(m_sorter)) return;

		auto sorter = value_ptr_sorter_type(m_sorter);
		auto comp = viewed::make_indirect_fun(std::move(sorter));
		varalgo::stable_sort(first, last, comp);
	}

	void FileTreeModel::stable_sort(value_ptr_iterator first, value_ptr_iterator last,
	                                int_vector::iterator ifirst, int_vector::iterator ilast)
	{
		if (not viewed::active(m_sorter)) return;

		auto sorter = value_ptr_sorter_type(m_sorter);
		auto comp = viewed::make_get_functor<0>(viewed::make_indirect_fun(std::move(sorter)));

		auto zfirst = ext::make_zip_iterator(first, ifirst);
		auto zlast = ext::make_zip_iterator(last, ilast);
		varalgo::stable_sort(zfirst, zlast, comp);
	}

	void FileTreeModel::sort_and_notify()
	{
		resort_context ctx;
		int_vector index_array, inverse_buffer_array;
		value_ptr_vector valptr_array;

		ctx.index_array   = &index_array;
		ctx.inverse_array = &inverse_buffer_array;
		ctx.vptr_array    = &valptr_array;

		layoutAboutToBeChanged(viewed::AbstractItemModel::empty_model_list, NoLayoutChangeHint);

		auto indexes = persistentIndexList();
		ctx.model_index_first = indexes.begin();
		ctx.model_index_last = indexes.end();

		sort_and_notify(m_root, ctx);

		layoutChanged(viewed::AbstractItemModel::empty_model_list, NoLayoutChangeHint);
	}

	void FileTreeModel::sort_and_notify(page_type & page, resort_context & ctx)
	{
		auto & container = page.children;
		auto & seq_view = container.get<by_seq>();
		auto seq_ptr_view = seq_view | ext::outdirected;
		constexpr int offset = 0;

		value_ptr_vector & valptr_vector = *ctx.vptr_array;
		int_vector & index_array = *ctx.index_array;
		int_vector & inverse_array = *ctx.inverse_array;

		valptr_vector.assign(seq_ptr_view.begin(), seq_ptr_view.end());
		index_array.resize(seq_ptr_view.size());

		auto first = valptr_vector.begin();
		auto middle = first + page.upassed;
		auto last = valptr_vector.end();

		auto ifirst = index_array.begin();
		auto imiddle = ifirst + page.upassed;
		auto ilast = index_array.end();

		std::iota(ifirst, ilast, offset);
		stable_sort(first, middle, ifirst, imiddle);
		
		seq_view.rearrange(boost::make_transform_iterator(first, make_ref));
		inverse_index_array(inverse_array, ifirst, ilast, offset);
		change_indexes(page, ctx.model_index_first, ctx.model_index_last,
		               inverse_array.begin(), inverse_array.end(), offset);

		for_each_child(page, [this, &ctx](auto & page) { sort_and_notify(page, ctx); });
	}

	void FileTreeModel::refilter_and_notify(viewed::refilter_type rtype)
	{
		switch (rtype)
		{
			default:
			case viewed::refilter_type::same:        return;
			
			case viewed::refilter_type::incremental: return refilter_incremental_and_notify();
			case viewed::refilter_type::full:        return refilter_full_and_notify();
		}
	}

	void FileTreeModel::refilter_incremental_and_notify()
	{
		refilter_context ctx;
		int_vector index_array, inverse_buffer_array;
		value_ptr_vector valptr_array;

		ctx.index_array = &index_array;
		ctx.inverse_array = &inverse_buffer_array;
		ctx.vptr_array = &valptr_array;

		layoutAboutToBeChanged(viewed::AbstractItemModel::empty_model_list, NoLayoutChangeHint);

		auto indexes = persistentIndexList();
		ctx.model_index_first = indexes.begin();
		ctx.model_index_last = indexes.end();

		refilter_incremental_and_notify(m_root, ctx);

		layoutChanged(viewed::AbstractItemModel::empty_model_list, NoLayoutChangeHint);
	}

	void FileTreeModel::refilter_incremental_and_notify(page_type & page, refilter_context & ctx)
	{
		for_each_child(page, [this, &ctx](auto & page) { refilter_incremental_and_notify(page, ctx); });

		auto & container = page.children;
		auto & seq_view = container.get<by_seq>();
		auto seq_ptr_view = seq_view | ext::outdirected;
		constexpr int offset = 0;

		value_ptr_vector & valptr_vector = *ctx.vptr_array;
		int_vector & index_array = *ctx.index_array;
		int_vector & inverse_array = *ctx.inverse_array;

		valptr_vector.assign(seq_ptr_view.begin(), seq_ptr_view.end());
		index_array.resize(seq_ptr_view.size());

		auto filter = value_ptr_filter_type(std::cref(m_filter));
		auto fpred = viewed::make_indirect_fun(std::move(filter));
		auto zfpred = viewed::make_get_functor<0>(fpred);

		auto vfirst = valptr_vector.begin();
		auto vlast = vfirst + page.upassed;

		auto ivfirst = index_array.begin();
		auto ivlast = ivfirst + page.upassed;

		std::iota(ivfirst, ivlast, offset);

		auto[vpp, ivpp] = std::stable_partition(
			ext::make_zip_iterator(vfirst, ivfirst),
			ext::make_zip_iterator(vlast, ivlast),
			zfpred).get_iterator_tuple();

		auto mark_index = [](int idx) { return viewed::mark_index(idx); };
		std::transform(ivpp, ivlast, ivpp, mark_index);

		int upassed_new = vpp - vfirst;
		seq_view.rearrange(boost::make_transform_iterator(vfirst, make_ref));
		page.upassed = upassed_new;

		inverse_index_array(inverse_array, index_array.begin(), index_array.end(), offset);
		change_indexes(page, ctx.model_index_first, ctx.model_index_last,
					   inverse_array.begin(), inverse_array.end(), offset);
	}

	void FileTreeModel::refilter_full_and_notify()
	{
		refilter_context ctx;
		int_vector index_array, inverse_buffer_array;
		value_ptr_vector valptr_array;

		ctx.index_array = &index_array;
		ctx.inverse_array = &inverse_buffer_array;
		ctx.vptr_array = &valptr_array;

		layoutAboutToBeChanged(viewed::AbstractItemModel::empty_model_list, NoLayoutChangeHint);

		auto indexes = persistentIndexList();
		ctx.model_index_first = indexes.begin();
		ctx.model_index_last = indexes.end();

		refilter_full_and_notify(m_root, ctx);

		layoutChanged(viewed::AbstractItemModel::empty_model_list, NoLayoutChangeHint);
	}

	void FileTreeModel::refilter_full_and_notify(page_type & page, refilter_context & ctx)
	{
		for_each_child(page, [this, &ctx](auto & page) { refilter_full_and_notify(page, ctx); });

		auto & container = page.children;
		auto & seq_view = container.get<by_seq>();
		auto seq_ptr_view = seq_view | ext::outdirected;
		constexpr int offset = 0;
		int upassed_new;

		value_ptr_vector & valptr_vector = *ctx.vptr_array;
		int_vector & index_array = *ctx.index_array;
		int_vector & inverse_array = *ctx.inverse_array;

		valptr_vector.assign(seq_ptr_view.begin(), seq_ptr_view.end());
		index_array.resize(seq_ptr_view.size());

		auto filter = value_ptr_filter_type(std::cref(m_filter));
		auto fpred = viewed::make_indirect_fun(std::move(filter));
		auto zfpred = viewed::make_get_functor<0>(fpred);

		auto vfirst = valptr_vector.begin();
		auto vlast = vfirst + page.upassed;
		auto sfirst = vlast;
		auto slast = valptr_vector.end();

		auto ivfirst = index_array.begin();
		auto ivlast = ivfirst + page.upassed;
		auto isfirst = ivlast;
		auto islast = index_array.end();

		std::iota(ivfirst, islast, offset);

		if (not viewed::active(m_filter))
		{
			upassed_new = slast - vfirst;
			merge_newdata(vfirst, vlast, slast, ivfirst, ivlast, islast, false);
		}
		else
		{
			auto[vpp, ivpp] = std::stable_partition(
				ext::make_zip_iterator(vfirst, ivfirst),
				ext::make_zip_iterator(vlast, ivlast),
				zfpred).get_iterator_tuple();

			auto[spp, ispp] = std::partition(
				ext::make_zip_iterator(sfirst, isfirst),
				ext::make_zip_iterator(slast, islast),
				zfpred).get_iterator_tuple();

			auto mark_index = [](int idx) { return viewed::mark_index(idx); };
			std::transform(ivpp, ivlast, ivpp, mark_index);
			std::transform(ispp, islast, ispp, mark_index);

			vlast = std::rotate(vpp, sfirst, spp);
			ivlast = std::rotate(ivpp, isfirst, ispp);
			
			upassed_new = vlast - vfirst;
			merge_newdata(vfirst, vpp, vlast, ivfirst, ivpp, ivlast, false);
		}

		seq_view.rearrange(boost::make_transform_iterator(vfirst, make_ref));
		page.upassed = upassed_new;

		inverse_index_array(inverse_array, index_array.begin(), index_array.end(), offset);
		change_indexes(page, ctx.model_index_first, ctx.model_index_last,
					   inverse_array.begin(), inverse_array.end(), offset);
	}

	auto FileTreeModel::copy_context(const upsert_context & ctx, QStringRef newprefix) -> upsert_context
	{
		upsert_context newctx;
		newctx.inserted_first = ctx.inserted_first;
		newctx.inserted_last = ctx.inserted_last;
		newctx.updated_first = ctx.updated_first;
		newctx.updated_last = ctx.updated_last;
		newctx.erased_first = ctx.erased_first;
		newctx.erased_last = ctx.erased_last;

		newctx.changed_first = newctx.changed_last = ctx.changed_first;
		newctx.removed_last = newctx.removed_first = ctx.removed_last;

		newctx.prefix = std::move(newprefix);		

		newctx.vptr_array = ctx.vptr_array;
		newctx.index_array = ctx.index_array;
		newctx.inverse_array = ctx.inverse_array;

		newctx.model_index_first = ctx.model_index_first;
		newctx.model_index_last = ctx.model_index_last;

		return newctx;
	}

	auto FileTreeModel::process_inserted(page_type & page, upsert_context & ctx)
		-> std::tuple<QString &, QStringRef &>
	{
		auto & container = page.children;
		//auto & seq_view = container.get<by_seq>();
		//auto & code_view = container.get<by_code>();
		
		// consumed nothing from previous step, return same name/prefix
		if (ctx.inserted_diff == 0)
			return std::tie(ctx.inserted_name, ctx.inserted_prefix);

		for (; ctx.inserted_first != ctx.inserted_last; ++ctx.inserted_first)
		{
			const auto * item = *ctx.inserted_first;
			std::uintptr_t type;
			std::tie(type, ctx.inserted_name, ctx.inserted_prefix) = analyze(ctx.prefix, *item);
			if (type == PAGE) return std::tie(ctx.inserted_name, ctx.inserted_prefix);

			bool inserted;
			std::tie(std::ignore, inserted) = container.insert(item);
			assert(inserted); (void)inserted;	
		}

		ctx.inserted_name = QString::null;
		ctx.inserted_prefix = {};
		return std::tie(ctx.inserted_name, ctx.inserted_prefix);
	}

	auto FileTreeModel::process_updated(page_type & page, upsert_context & ctx)
		-> std::tuple<QString &, QStringRef &>
	{
		auto & container = page.children;
		auto & seq_view = container.get<by_seq>();
		auto & code_view = container.get<by_code>();

		// consumed nothing from previous step, return same name/prefix
		if (ctx.updated_diff == 0)
			return std::tie(ctx.updated_name, ctx.updated_prefix);

		for (; ctx.updated_first != ctx.updated_last; ++ctx.updated_first)
		{
			const auto * item = *ctx.updated_first;
			std::uintptr_t type;
			std::tie(type, ctx.updated_name, ctx.updated_prefix) = analyze(ctx.prefix, *item);
			if (type == PAGE) return std::tie(ctx.updated_name, ctx.updated_prefix);

			auto it = container.find(get_name(item));
			assert(it != container.end());

			auto seqit = container.project<by_seq>(it);
			auto pos = seqit - seq_view.begin();
			*--ctx.changed_first = pos;
		}

		ctx.updated_name = QString::null;
		ctx.updated_prefix = {};
		return std::tie(ctx.updated_name, ctx.updated_prefix);
	}

	auto FileTreeModel::process_erased(page_type & page, upsert_context & ctx)
		-> std::tuple<QString &, QStringRef &>
	{
		auto & container = page.children;
		auto & seq_view = container.get<by_seq>();
		auto & code_view = container.get<by_code>();

		// consumed nothing from previous step, return same name/prefix
		if (ctx.erased_diff == 0)
			return std::tie(ctx.erased_name, ctx.erased_prefix);

		for (; ctx.erased_first != ctx.erased_last; ++ctx.erased_first)
		{
			const auto * item = *ctx.erased_first;
			std::uintptr_t type;
			std::tie(type, ctx.erased_name, ctx.erased_prefix) = analyze(ctx.prefix, *item);
			if (type == PAGE) return std::tie(ctx.erased_name, ctx.erased_prefix);

			auto it = container.find(get_name(item));
			assert(it != container.end());
			
			auto seqit = container.project<by_seq>(it);
			auto pos = seqit - seq_view.begin();
			*ctx.removed_last++ = pos;

			// erasion will be done later, in rearrange_children
			// container.erase(it);
		}

		ctx.erased_name = QString::null;
		ctx.erased_prefix = {};
		return std::tie(ctx.erased_name, ctx.erased_prefix);
	}

	void FileTreeModel::update_page(page_type & page, upsert_context & ctx)
	{
		const auto & prefix = ctx.prefix;
		auto & container = page.children;
		auto & seq_view  = container.get<by_seq>();
		auto & code_view = container.get<by_code>();
		auto oldsz = container.size();
		ctx.inserted_diff = ctx.updated_diff = ctx.erased_diff = -1;

		for (;;)
		{
			process_inserted(page, ctx);
			process_updated(page, ctx);
			process_erased(page, ctx);

			// at this point only pages are at front of ranges
			auto newprefix = std::max({ctx.erased_prefix, ctx.updated_prefix, ctx.inserted_prefix});
			auto name = std::max({ctx.erased_name, ctx.updated_name, ctx.inserted_name});
			if (name.isEmpty()) break;

			auto newctx = copy_context(ctx, std::move(newprefix));

			auto issub = viewed::make_indirect_fun(std::bind(is_subelement, std::cref(prefix), std::cref(name), std::placeholders::_1));
			ctx.inserted_first = std::find_if_not(ctx.inserted_first, ctx.inserted_last, issub);
			ctx.updated_first  = std::find_if_not(ctx.updated_first,  ctx.updated_last,  issub);
			ctx.erased_first   = std::find_if_not(ctx.erased_first,   ctx.erased_last,   issub);

			newctx.inserted_last = ctx.inserted_first;
			newctx.updated_last  = ctx.updated_first;
			newctx.erased_last   = ctx.erased_first;

			ctx.inserted_diff = newctx.inserted_last - newctx.inserted_first;
			ctx.updated_diff  = newctx.updated_last  - newctx.updated_first;
			ctx.erased_diff   = newctx.erased_last   - newctx.erased_first;
			assert(ctx.inserted_diff or ctx.updated_diff or ctx.erased_diff);

			page_type * child_page = nullptr;
			bool inserted = false;
			auto it = container.find(name);
			if (it != container.end())
				child_page = static_cast<page_type *>(it->pointer());
			else 
			{
				assert(ctx.updated_diff or ctx.inserted_diff);
				auto child = std::make_unique<page_type>();
				child_page = child.get();

				child_page->name = name;
				child_page->parent = &page;
				std::tie(it, inserted) = container.insert(std::move(child));
			}			

			// process child
			if (child_page)
			{
				update_page(*child_page, newctx);

				auto seqit = container.project<by_seq>(it);
				auto pos = seqit - seq_view.begin();

				if (child_page->children.size() == 0)
					// actual erasion will be done later in rearrange_children
					*ctx.removed_last++ = pos;
				else if (not inserted)
					*--ctx.changed_first = pos;
			}
		}

		ctx.inserted_count = container.size() - oldsz;
		ctx.updated_count = ctx.changed_last - ctx.changed_first;
		ctx.erased_count = ctx.removed_last - ctx.removed_first;

		rearrange_children(page, ctx);
		recalculate_page(page);
	}

	void FileTreeModel::rearrange_children(page_type & page, upsert_context & ctx)
	{
		auto & container = page.children;
		auto & seq_view = container.get<by_seq>();
		auto seq_ptr_view = seq_view | ext::outdirected;
		constexpr int offset = 0;
		int upassed_new;

		value_ptr_vector & valptr_vector = *ctx.vptr_array;
		int_vector & index_array = *ctx.index_array;
		int_vector & inverse_array = *ctx.inverse_array;

		valptr_vector.assign(seq_ptr_view.begin(), seq_ptr_view.end());
		auto vfirst = valptr_vector.begin();
		auto vlast = vfirst + page.upassed;
		auto sfirst = vlast;
		auto slast = vfirst + (seq_ptr_view.size() - ctx.inserted_count);
		auto nfirst = slast;
		auto nlast = valptr_vector.end();

		index_array.resize(container.size());
		auto ifirst = index_array.begin();
		auto imiddle = ifirst + page.upassed;
		auto ilast  = index_array.end();
		std::iota(ifirst, ilast, offset);

		auto filter = value_ptr_filter_type(std::cref(m_filter));
		auto fpred = viewed::make_indirect_fun(std::move(filter));
		auto index_pass_pred = [vfirst, fpred](int index) { return fpred(vfirst[index]); };


		auto vchanged_first = ctx.changed_first;
		auto vchanged_last = std::partition(ctx.changed_first, ctx.changed_last,
			[upassed = page.upassed](int index) { return index < upassed; });

		auto vchanged_pp = viewed::active(m_filter) 
			? std::partition(vchanged_first, vchanged_last, index_pass_pred)
			: vchanged_last;

		auto schanged_first = vchanged_last;
		auto schanged_last = ctx.changed_last;

		// mark removed ones by nullifying them
		for (auto it = ctx.removed_first; it != ctx.removed_last; ++it)
		{
			int index = *it;
			vfirst[index] = nullptr;
			ifirst[index] = -1;
		}

		// mark removed ones from [vfirst, vlast) by nullifying them
		for (auto it = vchanged_pp; it != vchanged_last; ++it)
		{
			int index = *it;
			vfirst[index] = nullptr;
			ifirst[index] = -1;
		}

		if (not viewed::active(m_filter))
		{
			// remove erased ones, and filtered out ones
			vlast  = std::remove(vfirst, vlast, nullptr);
			sfirst = std::remove(std::make_reverse_iterator(slast), std::make_reverse_iterator(sfirst), nullptr).base();
			nlast  = std::move(sfirst, nlast, vlast);
			upassed_new = nlast - vfirst;
		}
		else
		{
			for (auto it = schanged_first; it != schanged_last; ++it)
				vfirst[*it] = mark_pointer(vfirst[*it]);

			vlast  = std::remove_if(vfirst, vlast, [](auto ptr) { return unmark_pointer(ptr) == nullptr; });
			sfirst = std::remove(std::make_reverse_iterator(slast), std::make_reverse_iterator(sfirst), nullptr).base();

			// [spp, npp) - gathered elements from [sfirst, nlast) satisfying fpred
			auto spp = std::partition(sfirst, slast, [](auto * ptr) { return not marked_pointer(ptr); });
			auto npp = std::partition(nfirst, nlast, fpred);
			upassed_new = (vlast - vfirst) + (npp - spp);

			for (auto it = spp; it != slast; ++it)
				*it = unmark_pointer(*it);

			// rotate them at the beginning of shadow area
			// and in fact merge those with visible area
			std::rotate(sfirst, spp, nlast);
			nlast = std::move(sfirst, nlast, vlast);
		}

		auto rfirst = nlast;
		auto rlast = rfirst;

		{
			auto point = imiddle;
			imiddle = std::remove(ifirst, imiddle, -1);
			ilast = std::remove(point, ilast, -1);			
			ilast = std::move(point, ilast, imiddle);
		}
		

		for (auto it = vchanged_pp; it != vchanged_last; ++it)
		{
			int index = *it;
			*rlast++ = seq_ptr_view[index];
			*ilast++ = viewed::mark_index(index);
		}

		for (auto it = ctx.removed_first; it != ctx.removed_last; ++it)
		{
			int index = *it;
			*rlast++ = seq_ptr_view[index];
			*ilast++ = viewed::mark_index(index);
		}

		bool resort_old = vchanged_first != vchanged_pp;
		merge_newdata(vfirst, vlast, nlast, ifirst, imiddle, ifirst + (nlast - vfirst), resort_old);

		seq_view.rearrange(boost::make_transform_iterator(vfirst, make_ref));
		seq_view.resize(seq_view.size() - ctx.erased_count);
		page.upassed = upassed_new;
		
		inverse_index_array(inverse_array, ifirst, ilast, offset);
		change_indexes(page, ctx.model_index_first, ctx.model_index_last,
		               inverse_array.begin(), inverse_array.end(), offset);
	}

	void FileTreeModel::recalculate_page(page_type & page)
	{
		constexpr size_type zero = 0;
		auto & children = page.children.get<by_seq>();
		auto first = children.begin();
		auto last  = children.end();
		
		page.total_size = std::accumulate(first, last, zero, [](size_type val, auto & item) { return val + get_total_size(item); });
		page.have_size = std::accumulate(first, last, zero, [](size_type val, auto & item) { return val + get_have_size(item); });
	}

	void FileTreeModel::update_data(const signal_range_type & erased, const signal_range_type & updated, const signal_range_type & inserted)
	{
		int_vector affected_indexes, index_array, inverse_buffer_array;
		value_ptr_vector valptr_array;
		//leaf_ptr_vector updated_vec, inserted_vec, erased_vec;
		
		affected_indexes.resize(erased.size() + std::max(updated.size(), inserted.size()));
		//updated_vec.assign(updated.begin(), updated.end());
		//erased_vec.assign(erased.begin(), erased.end());
		//inserted_vec.assign(inserted.begin(), inserted.end());
		
		upsert_context ctx;
		ctx.index_array = &index_array;
		ctx.inverse_array = &inverse_buffer_array;
		ctx.vptr_array = &valptr_array;

		//ctx.erased_first = erased_vec.begin();
		//ctx.erased_last  = erased_vec.end();
		//ctx.inserted_first = inserted_vec.begin();
		//ctx.inserted_last = inserted_vec.end();
		//ctx.updated_first = updated_vec.begin();
		//ctx.updated_last = updated_vec.end();

		ctx.erased_first = erased.begin();
		ctx.erased_last  = erased.end();
		ctx.inserted_first = inserted.begin();
		ctx.inserted_last = inserted.end();
		ctx.updated_first = updated.begin();
		ctx.updated_last = updated.end();

		ctx.removed_first = ctx.removed_last = affected_indexes.begin();
		ctx.changed_first = ctx.changed_last = affected_indexes.end();

		group_by_dirs(ctx.erased_first, ctx.erased_last);
		group_by_dirs(ctx.inserted_first, ctx.inserted_last);
		group_by_dirs(ctx.updated_first, ctx.updated_last);
		
		layoutAboutToBeChanged(viewed::AbstractItemModel::empty_model_list, NoLayoutChangeHint);
		
		auto indexes = persistentIndexList();
		ctx.model_index_first = indexes.begin();
		ctx.model_index_last  = indexes.end();

		update_page(m_root, ctx);

		layoutChanged(viewed::AbstractItemModel::empty_model_list, NoLayoutChangeHint);
	}

	void FileTreeModel::erase_records(const signal_range_type & erased)
	{
		signal_range_type inserted, updated;
		update_data(erased, updated, inserted);
	}

	void FileTreeModel::clear_view()
	{
		beginResetModel();
		m_root.children.clear();
		endResetModel();
	}

	void FileTreeModel::FilterBy(QString expr)
	{
		auto rtype = m_tfilt.set_expr(expr);
		refilter_and_notify(rtype);
	}

	void FileTreeModel::init()
	{
		connect_signals();
		reinit_view();
	}

	void FileTreeModel::connect_signals()
	{
		m_clear_con = m_owner->on_clear([this] { clear_view(); });
		m_update_con = m_owner->on_update([this](const signal_range_type & e, const signal_range_type & u, const signal_range_type & i) { update_data(e, u, i); });
		m_erase_con = m_owner->on_erase([this](const signal_range_type & r) { erase_records(r); });
	}

	FileTreeModel::FileTreeModel(std::shared_ptr<torrent_file_store> store, QObject * parent /* = nullptr */)
		: base_type(parent), m_owner(std::move(store))
	{
		init();
	}
}
