#include <ext/utility.hpp>
#include <ext/iterator/outdirect_iterator.hpp>
#include <ext/iterator/indirect_iterator.hpp>
#include <ext/range/adaptors/outdirected.hpp>
#include <boost/range/counting_range.hpp>

#include <QtTools/ToolsBase.hpp>
#include <qtor/FileTreeModel.hqt>

namespace qtor
{
	const FileTreeModel::value_container FileTreeModel::ms_empty_container;

	inline void FileTreeModel::value_ptr::destroy() noexcept
	{
		if (owning)
		{
			if (type == PAGE)
				delete reinterpret_cast<page_type *>(ptr);
			else
				delete reinterpret_cast<leaf_type *>(ptr);
		}
	}

	inline FileTreeModel::value_ptr::value_ptr() noexcept
	{
		this->val = 0;
	}

	inline FileTreeModel::value_ptr::~value_ptr() noexcept
	{
		destroy();
	}

	inline FileTreeModel::value_ptr::value_ptr(value_ptr && op) noexcept
	{
		val = std::exchange(op.val, 0);
	}

	inline auto FileTreeModel::value_ptr::operator =(value_ptr && op) noexcept -> value_ptr &
	{
		if (this != &op)
		{
			std::destroy_at(this);
			new(this) value_ptr(std::move(op));
		}

		return *this;
	}

	inline FileTreeModel::value_ptr::value_ptr(const leaf_type * ptr) noexcept
	{
		this->owning = 0;
		this->type = LEAF;
		this->ptr = reinterpret_cast<std::uintptr_t>(ptr);
	}

	inline FileTreeModel::value_ptr::value_ptr(const page_type * ptr) noexcept
	{
		this->owning = 0;
		this->type = PAGE;
		this->ptr = reinterpret_cast<std::uintptr_t>(ptr);
	}

	inline auto FileTreeModel::value_ptr::operator =(const leaf_type * ptr) noexcept -> value_ptr &
	{
		destroy();

		this->owning = 0;
		this->type = LEAF;
		this->ptr = reinterpret_cast<std::uintptr_t>(ptr);

		return *this;
	}

	inline auto FileTreeModel::value_ptr::operator =(const page_type * ptr) noexcept -> value_ptr &
	{
		destroy();

		this->owning = 0;
		this->type = PAGE;
		this->ptr = reinterpret_cast<std::uintptr_t>(ptr);

		return *this;
	}

	template <class Type>
	inline FileTreeModel::value_ptr::value_ptr(std::unique_ptr<Type> ptr) noexcept
		: value_ptr(ptr.release())
	{
		static_assert(not std::is_const_v<Type>);
		this->owning = 1;
	}

	template <class Type>
	inline auto FileTreeModel::value_ptr::operator =(std::unique_ptr<Type> ptr) noexcept -> value_ptr &
	{
		destroy();

		static_assert(not std::is_const_v<Type>);
		this->owning = 1;
		return operator =(ptr.release());
	}

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

	filepath_type FileTreeModel::get_name_type::operator()(const leaf_type * ptr) const
	{
		int pos = ptr->filename.lastIndexOf('/') + 1;
		return ptr->filename.mid(pos);
	}

	filepath_type FileTreeModel::get_name_type::operator()(const page_type * ptr) const
	{
		return ptr->name;
	}

	filepath_type FileTreeModel::get_name_type::operator()(const value_ptr & val) const
	{
		return val.visit(*this);
	}

	int FileTreeModel::columnCount(const QModelIndex & parent /* = QModelIndex() */) const
	{
		return 1;
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
			assert(element.type == PAGE);
			auto * page = reinterpret_cast<page_type *>(element.ptr);
			return createIndex(row, column, page);
		}
	}

	QVariant FileTreeModel::data(const QModelIndex & index, int role /* = Qt::DisplayRole */) const
	{
		if (not index.isValid()) return {};

		const auto & val = get_element_ptr(index);
		switch (role)
		{
			case Qt::DisplayRole:
			case Qt::ToolTipRole:
				return get_name(val);

			default:
				return {};
		}
	}

	QVariant FileTreeModel::headerData(int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */) const
	{
		if (section == 0 and orientation == Qt::Horizontal and role == Qt::DisplayRole)
			return QStringLiteral("fname");

		return {};
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
			inverse[std::abs(val) - offset] = val >= 0 ? i : -1;
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

	void FileTreeModel::init()
	{
		connect_signals();
		reinit_view();
	}

	void FileTreeModel::connect_signals()
	{
		m_clear_con  = m_owner->on_clear([this] { clear_view(); });
		m_update_con = m_owner->on_update([this](const signal_range_type & e, const signal_range_type & u, const signal_range_type & i) { update_data(e, u, i); });
		m_erase_con  = m_owner->on_erase([this](const signal_range_type & r) { erase_records(r); });
	}

	void FileTreeModel::reinit_view()
	{
		beginResetModel();

		m_root.upassed = 0;
		m_root.children.clear();

		std::vector<signal_range_type::value_type> elements;
		elements.assign(
			ext::make_outdirect_iterator(m_owner->begin()),
			ext::make_outdirect_iterator(m_owner->end())
		);

		auto first = elements.begin();
		auto last  = elements.end();
		std::sort(first, last, [](auto & v1, auto & v2) { return v1->filename < v2->filename; });
		fill_page(m_root, {}, first, last);

		endResetModel();
	}
	
	template <class ForwardIterator>
	void FileTreeModel::fill_page(page_type & page, QStringRef prefix, ForwardIterator first, ForwardIterator last)
	{
		while (first != last)
		{
			auto item_ptr = *first;
			auto [type, name, newprefix] = analyze(prefix, *item_ptr);
			if (type == LEAF)
			{
				auto * leaf = item_ptr;
				page.children.insert(leaf);
				++first;
			}
			else // PAGE
			{
				auto page_ptr = std::make_unique<page_type>();
				page_ptr->parent = &page;
				page_ptr->name = name;

				auto issub = std::bind(is_subelement, std::cref(prefix), std::cref(name), std::placeholders::_1);
				auto it = std::find_if_not(first, last, viewed::make_indirect_fun(std::move(issub)));
				fill_page(*page_ptr, std::move(newprefix), first, it);
				page.children.insert(std::move(page_ptr));
				first = it;
			}
		}

		page.upassed = page.children.size();
		sort_children(page);
	}

	void FileTreeModel::merge_newdata(value_ptr_iterator first, value_ptr_iterator middle, value_ptr_iterator last, bool resort_old /* = true */)
	{
		if (not viewed::active(m_sorter)) return;

		auto comp = viewed::make_indirect_fun(m_sorter);

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

		auto comp = viewed::make_get_functor<0>(viewed::make_indirect_fun(m_sorter));

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

		auto comp = viewed::make_indirect_fun(m_sorter);
		varalgo::stable_sort(first, last, comp);
	}

	void FileTreeModel::stable_sort(value_ptr_iterator first, value_ptr_iterator last,
	                                int_vector::iterator ifirst, int_vector::iterator ilast)
	{
		if (not viewed::active(m_sorter)) return;

		auto comp = viewed::make_get_functor<0>(viewed::make_indirect_fun(m_sorter));

		auto zfirst = ext::make_zip_iterator(first, ifirst);
		auto zlast = ext::make_zip_iterator(last, ilast);
		varalgo::stable_sort(zfirst, zlast, comp);
	}

	void FileTreeModel::sort_and_notify()
	{
		sort_context ctx;
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

	void FileTreeModel::sort_and_notify(page_type & page, sort_context & ctx)
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
		
		seq_view.rearrange(boost::make_transform_iterator(first, [](auto * ptr) { return std::ref(*ptr); }));
		inverse_index_array(inverse_array, ifirst, ilast, offset);
		change_indexes(page, ctx.model_index_first, ctx.model_index_last,
		               inverse_array.begin(), inverse_array.end(), offset);

		for (auto & child : page.children) 
		{
			if (child.type == PAGE)
			{
				auto * child_page = reinterpret_cast<page_type *>(child.ptr);
				sort_and_notify(*child_page, ctx);
			}
		}
	}


	auto FileTreeModel::copy_context(const processing_context & ctx, QStringRef newprefix) -> processing_context
	{
		auto newctx = ctx;
		newctx.prefix = std::move(newprefix);
		newctx.changed_first = newctx.changed_last;
		newctx.removed_last = newctx.removed_first;

		return newctx;
	}

	auto FileTreeModel::process_inserted(page_type & page, processing_context & ctx)
		-> std::tuple<QString, QStringRef>
	{
		auto & container = page.children;
		//auto & seq_view = container.get<by_seq>();
		//auto & code_view = container.get<by_code>();

		for (; ctx.inserted_first != ctx.inserted_last; ++ctx.inserted_first)
		{
			const auto * item = *ctx.inserted_first;
			auto [type, name, prefix] = analyze(ctx.prefix, *item);
			if (type == PAGE) return {std::move(name), std::move(prefix)};

			bool inserted;
			std::tie(std::ignore, inserted) = container.insert(item);
			assert(inserted); (void)inserted;	
		}

		return {QString::null, QStringRef()};
	}

	auto FileTreeModel::process_updated(page_type & page, processing_context & ctx)
		-> std::tuple<QString, QStringRef>
	{
		auto & container = page.children;
		auto & seq_view = container.get<by_seq>();
		auto & code_view = container.get<by_code>();

		for (; ctx.updated_first != ctx.updated_last; ++ctx.updated_first)
		{
			const auto * item = *ctx.updated_first;
			auto [type, name, prefix] = analyze(ctx.prefix, *item);
			if (type == PAGE) return {std::move(name), std::move(prefix)};

			auto it = container.find(get_name(item));
			assert(it != container.end());

			auto seqit = container.project<by_seq>(it);
			auto pos = seqit - seq_view.begin();
			*--ctx.changed_first = pos;
		}

		return {QString::null, QStringRef()};
	}

	auto FileTreeModel::process_erased(page_type & page, processing_context & ctx)
		-> std::tuple<QString, QStringRef>
	{
		auto & container = page.children;
		auto & seq_view = container.get<by_seq>();
		auto & code_view = container.get<by_code>();

		for (; ctx.erased_first != ctx.erased_last; ++ctx.erased_first)
		{
			const auto * item = *ctx.erased_first;
			auto [type, name, prefix] = analyze(ctx.prefix, *item);
			if (type == PAGE) return {std::move(name), std::move(prefix)};

			auto it = container.find(get_name(item));
			assert(it != container.end());
			
			auto seqit = container.project<by_seq>(it);
			auto pos = seqit - seq_view.begin();
			*ctx.removed_last++ = pos;

			// erasion will be done later, in AMOLED
			// container.erase(it);
		}

		return {QString::null, QStringRef()};
	}

	void FileTreeModel::update_page(page_type & page, processing_context & ctx)
	{
		const auto & prefix = ctx.prefix;
		auto & container = page.children;
		auto & seq_view  = container.get<by_seq>();
		auto & code_view = container.get<by_code>();
		auto oldsz = container.size();

		for (;;)
		{
			QStringRef erased_newprefix, inserted_newprefix, updated_newprefix;
			QString erased_name, inserted_name, updated_name;

			std::tie(inserted_name, inserted_newprefix) = process_inserted(page, ctx);
			std::tie(updated_name, updated_newprefix) = process_updated(page, ctx);
			std::tie(erased_name, erased_newprefix) = process_erased(page, ctx);

			// at this point only pages are at front of ranges
			auto newprefix = std::max({erased_newprefix, updated_newprefix, inserted_newprefix});
			auto name = std::max({erased_name, updated_name, inserted_name});
			if (name.isEmpty()) break;

			auto newctx = copy_context(ctx, std::move(newprefix));

			auto issub = viewed::make_indirect_fun(std::bind(is_subelement, std::cref(prefix), std::cref(name), std::placeholders::_1));
			ctx.inserted_first = std::find_if_not(ctx.inserted_first, ctx.inserted_last, issub);
			ctx.updated_first  = std::find_if_not(ctx.updated_first,  ctx.updated_last,  issub);
			ctx.erased_first   = std::find_if_not(ctx.erased_first,   ctx.erased_last,   issub);

			newctx.inserted_last = ctx.inserted_first;
			newctx.updated_last  = ctx.updated_first;
			newctx.erased_last   = ctx.erased_first;

			ctx.inserted_diff = newctx.inserted_first != newctx.inserted_last;
			ctx.updated_diff = newctx.updated_first != newctx.updated_last;
			ctx.erased_diff = newctx.erased_first != newctx.erased_last;
			assert(ctx.inserted_diff or ctx.updated_diff or ctx.erased_diff);

			page_type * child_page = nullptr;
			bool inserted = false;
			auto it = container.find(name);
			if (it != container.end())
				child_page = reinterpret_cast<page_type *>(it->ptr);				
			else 
			{
				assert(ctx.updated_diff or ctx.inserted_diff);
				auto child = std::make_unique<page_type>();
				child_page = child.get();

				child_page->name = name;
				child_page->parent = &page;
				std::tie(it, inserted) = page.children.insert(std::move(child));
			}			

			// process child
			if (child_page)
			{
				update_page(*child_page, newctx);

				auto seqit = container.project<by_seq>(it);
				auto pos = seqit - seq_view.begin();

				if (child_page->children.size() == 0)
					// erasion will be done later in AMOLED
					*ctx.removed_last++ = pos;
				else if (not inserted)
					*--ctx.changed_first = pos;
			}
		}

		ctx.inserted_count = container.size() - oldsz;
		ctx.updated_count = ctx.changed_last - ctx.changed_first;
		ctx.erased_count = ctx.removed_last - ctx.removed_first;

		super_AMOLED(page, ctx);
	}

	void FileTreeModel::super_AMOLED(page_type & page, processing_context & ctx)
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

		auto fpred = viewed::make_indirect_fun(std::cref(m_filter));
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
				mark_pointer(vfirst[*it]);

			vlast  = std::remove_if(vfirst, vlast, [](auto ptr) { return unmark_pointer(ptr) == nullptr; });
			sfirst = std::remove(std::make_reverse_iterator(slast), std::make_reverse_iterator(sfirst), nullptr).base();

			// [spp, npp) - gathered elements from [sfirst, nlast) satisfying fpred
			auto spp = std::partition(sfirst, slast, [](auto * ptr) { return not is_marked(ptr); });
			auto npp = std::partition(nfirst, nlast, fpred);
			upassed_new = vlast - vfirst + npp - spp;

			for (auto it = spp; it != slast; ++it)
				unmark_pointer(*it);			

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
			*ilast++ = -index;
		}

		for (auto it = ctx.removed_first; it != ctx.removed_last; ++it)
		{
			int index = *it;
			*rlast++ = seq_ptr_view[index];
			*ilast++ = -index;
		}

		bool resort_old = vchanged_first != vchanged_pp;
		merge_newdata(vfirst, vlast, nlast, ifirst, imiddle, ifirst + (nlast - vfirst), resort_old);

		seq_view.rearrange(boost::make_transform_iterator(vfirst, [](auto * ptr) { return std::ref(*ptr); }));
		seq_view.resize(seq_view.size() - ctx.erased_count);
		page.upassed = upassed_new;
		
		inverse_index_array(inverse_array, ifirst, ilast, offset);
		change_indexes(page, ctx.model_index_first, ctx.model_index_last,
		               inverse_array.begin(), inverse_array.end(), offset);
	}

	void FileTreeModel::update_data(const signal_range_type & sorted_erased, const signal_range_type & sorted_updated, const signal_range_type & inserted)
	{
		int_vector affected_indexes, index_array, inverse_buffer_array;
		value_ptr_vector valptr_array;
		leaf_ptr_vector updated_vec, inserted_vec, erased_vec;
		
		affected_indexes.resize(sorted_erased.size() + std::max(sorted_updated.size(), inserted.size()));
		updated_vec.assign(sorted_updated.begin(), sorted_updated.end());
		erased_vec.assign(sorted_erased.begin(), sorted_erased.end());
		inserted_vec.assign(inserted.begin(), inserted.end());
		
		processing_context ctx;
		ctx.index_array = &index_array;
		ctx.inverse_array = &inverse_buffer_array;
		ctx.vptr_array = &valptr_array;

		ctx.erased_first = erased_vec.begin();
		ctx.erased_last  = erased_vec.end();
		ctx.inserted_first = inserted_vec.begin();
		ctx.inserted_last = inserted_vec.end();
		ctx.updated_first = updated_vec.begin();
		ctx.updated_last = updated_vec.end();

		ctx.removed_first = ctx.removed_last = affected_indexes.begin();
		ctx.changed_first = ctx.changed_last = affected_indexes.end();

		auto sort_pred = viewed::make_indirect_fun(torrent_file_id_greater());
		std::sort(ctx.erased_first, ctx.erased_last, sort_pred);
		std::sort(ctx.inserted_first, ctx.inserted_last, sort_pred);
		std::sort(ctx.updated_first, ctx.updated_last, sort_pred);
		
		layoutAboutToBeChanged(viewed::AbstractItemModel::empty_model_list, NoLayoutChangeHint);
		
		auto indexes = persistentIndexList();
		ctx.model_index_first = indexes.begin();
		ctx.model_index_last  = indexes.end();

		update_page(m_root, ctx);

		layoutChanged(viewed::AbstractItemModel::empty_model_list, NoLayoutChangeHint);
	}

	void FileTreeModel::sort_children(page_type & page)
	{
		auto & seq_view = page.children.get<by_seq>();

		std::vector<std::reference_wrapper<const value_ptr>> refs;
		refs.assign(seq_view.begin(), seq_view.end());

		auto refs_first = refs.begin();
		auto refs_last = refs.end();
		std::sort(refs_first, refs_last, m_sorter);

		seq_view.rearrange(refs_first);
	}

	void FileTreeModel::erase_records(const signal_range_type & sorted_erased)
	{

	}

	void FileTreeModel::clear_view()
	{
		beginResetModel();
		m_root.children.clear();
		endResetModel();
	}

	FileTreeModel::FileTreeModel(std::shared_ptr<torrent_file_store> store, QObject * parent /* = nullptr */)
		: base_type(parent), m_owner(std::move(store))
	{
		init();
	}
}
