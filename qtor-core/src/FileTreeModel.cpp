#include <ext/utility.hpp>
#include <ext/iterator/outdirect_iterator.hpp>
#include <ext/iterator/indirect_iterator.hpp>
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
		assert(index.row() < seq_view.size());
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

	auto FileTreeModel::copy_context(const processing_context & ctx, QStringRef newprefix) -> processing_context
	{
		auto newctx = ctx;
		newctx.prefix = std::move(newprefix);
		newctx.changed_first = newctx.changed_last;
		newctx.removed_last = newctx.removed_first;

		return newctx;
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
			container.erase(it);
		}

		return {QString::null, QStringRef()};
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

	void FileTreeModel::update_page(page_type & page, processing_context & ctx)
	{
		const auto & prefix = ctx.prefix;
		auto & container = page.children;
		auto & seq_view  = container.get<by_seq>();
		auto & code_view = container.get<by_code>();

		for (;;)
		{
			QStringRef erased_newprefix, inserted_newprefix, updated_newprefix;
			QString erased_name, inserted_name, updated_name;

			std::tie(erased_name, erased_newprefix) = process_erased(page, ctx);
			std::tie(updated_name, updated_newprefix) = process_updated(page, ctx);
			std::tie(inserted_name, inserted_newprefix) = process_inserted(page, ctx);

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

			bool inserts = newctx.inserted_first != newctx.inserted_last;
			bool updates = newctx.updated_first != newctx.updated_last;
			bool erases  = newctx.erased_first != newctx.erased_last;
			assert(inserts or updates or erases);

			page_type * child_page = nullptr;
			auto it = container.find(name);
			if (it != container.end())
				child_page = reinterpret_cast<page_type *>(it->ptr);				
			else 
			{
				assert(updates or inserts);
				auto child = std::make_unique<page_type>();
				child_page = child.get();

				child_page->name = name;
				child_page->parent = &page;
				std::tie(it, std::ignore) = page.children.insert(std::move(child));
			}			

			// process child
			if (child_page)
			{
				update_page(*child_page, newctx);

				auto seqit = container.project<by_seq>(it);
				auto pos = seqit - seq_view.begin();

				if (/*it != container.end() and*/ child_page->children.size() == 0)
				{
					*ctx.removed_last++ = pos;

					// erasion will be done later in AMOLED
					container.erase(it);
				}
				else
				{
					*--ctx.changed_first = pos;
				}
			}
		}

		page.upassed = page.children.size();
		sort_children(page);
	}

	void FileTreeModel::update_data(const signal_range_type & sorted_erased, const signal_range_type & sorted_updated, const signal_range_type & inserted)
	{
		int_vector affected_indexes;
		leaf_ptr_vector updated_vec, inserted_vec, erased_vec;
		
		affected_indexes.resize(sorted_erased.size() + std::max(sorted_updated.size(), inserted.size()));
		updated_vec.assign(sorted_updated.begin(), sorted_updated.end());
		erased_vec.assign(sorted_erased.begin(), sorted_erased.end());
		inserted_vec.assign(inserted.begin(), inserted.end());
		
		processing_context ctx;

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

		beginResetModel();
		//layoutAboutToBeChanged({}, NoLayoutChangeHint);
		
		//auto indexes = persistentIndexList();
		//ctx.indexes = &indexes;

		update_page(m_root, ctx);

		endResetModel();
		//layoutChanged({}, NoLayoutChangeHint);
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

	void FileTreeModel::super_AMOLED(page_type & page, processing_context & ctx)
	{

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
