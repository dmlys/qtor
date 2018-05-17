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
				delete reinterpret_cast<page_ptr>(ptr);
			else
				delete reinterpret_cast<leaf_ptr>(ptr);
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

	inline FileTreeModel::value_ptr::value_ptr(leaf_ptr ptr) noexcept
	{
		this->owning = 0;
		this->type = LEAF;
		this->ptr = reinterpret_cast<std::uintptr_t>(ptr);
	}

	inline FileTreeModel::value_ptr::value_ptr(page_ptr ptr) noexcept
	{
		this->owning = 0;
		this->type = PAGE;
		this->ptr = reinterpret_cast<std::uintptr_t>(ptr);
	}

	inline auto FileTreeModel::value_ptr::operator =(leaf_ptr ptr) noexcept -> value_ptr &
	{
		destroy();

		this->owning = 0;
		this->type = LEAF;
		this->ptr = reinterpret_cast<std::uintptr_t>(ptr);

		return *this;
	}

	inline auto FileTreeModel::value_ptr::operator =(page_ptr ptr) noexcept -> value_ptr &
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
		this->owning = 1;
	}

	template <class Type>
	inline auto FileTreeModel::value_ptr::operator =(std::unique_ptr<Type> ptr) noexcept -> value_ptr &
	{
		destroy();

		this->owning = 1;
		return operator =(ptr.release());
	}

	inline auto FileTreeModel::get_page(const QModelIndex & index) const -> page_ptr
	{
		return static_cast<page_ptr>(index.internalPointer());
	}

	auto FileTreeModel::get_element_ptr(const QModelIndex & index) const -> const value_ptr &
	{
		assert(index.isValid());
		auto * page = get_page(index);
		assert(page);

		auto & seq_view = page->childs.get<by_seq>();
		assert(index.row() < seq_view.size());
		return seq_view[index.row()];
	}

	filepath_type FileTreeModel::get_name_type::operator()(leaf_ptr ptr) const
	{
		int pos = ptr->filename.lastIndexOf('/') + 1;
		return ptr->filename.mid(pos);
	}

	filepath_type FileTreeModel::get_name_type::operator()(page_ptr ptr) const
	{
		return ptr->node.name;
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
			return qint(m_root.childs.size());

		const auto & val = get_element_ptr(parent);
		const auto & children = get_children(val);
		return qint(children.size());
	}

	QModelIndex FileTreeModel::parent(const QModelIndex & index) const
	{
		if (not index.isValid()) return {};
		
		page_ptr page = get_page(index);
		assert(page);

		page_ptr parent_page = page->parent;
		if (not parent_page) return {}; // already top level index

		auto & children = parent_page->childs;
		auto & seq_view = children.get<by_seq>();

		auto code_it = children.find(page->node.name);
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

			if (children.size() < row)
				return {};

			// only page can have children
			assert(element.type == PAGE);
			auto * page = reinterpret_cast<page_ptr>(element.ptr);
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

	//void FileTreeModel::erase_records(const signal_range_type & sorted_erased)
	//{
	//	if (sorted_erased.empty()) return;

	//	int_vector affected_indexes(sorted_erased.size());
	//	auto erased_first = affected_indexes.begin();
	//	auto erased_last = erased_first;

	//	//auto & seq_view = m_

	//	for (auto it = std::find_if(first, last, test); it != last; it = std::find_if(++it, last, test))
	//		*erased_last++ = static_cast<int>(it - first);

	//	auto * model = get_model();
	//	Q_EMIT model->layoutAboutToBeChanged(model_type::empty_model_list, model->NoLayoutChangeHint);

	//	auto index_map = viewed::build_relloc_map(erased_first, erased_last, m_store.size());
	//	change_indexes(index_map.begin(), index_map.end(), 0);

	//	last = viewed::remove_indexes(first, last, erased_first, erased_last);
	//	m_store.resize(last - first);

	//	Q_EMIT model->layoutChanged(model_type::empty_model_list, model->NoLayoutChangeHint);
	//}

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

	void FileTreeModel::connect_signals()
	{
		m_clear_con  = m_owner->on_clear([this] { clear_view(); });
		m_update_con = m_owner->on_update([this](const signal_range_type & e, const signal_range_type & u, const signal_range_type & i) { update_data(e, u, i); });
		m_erase_con  = m_owner->on_erase([this](const signal_range_type & r) { erase_records(r); });
	}

	void FileTreeModel::reinit_view()
	{
		std::vector<signal_range_type::value_type> elements;
		elements.assign(
			ext::make_outdirect_iterator(m_owner->begin()),
			ext::make_outdirect_iterator(m_owner->end())
		);

		auto first = elements.begin();
		auto last  = elements.end();
		std::sort(first, last, [](auto & v1, auto & v2) { return v1->filename < v2->filename; });
		fill_page(m_root, {}, first, last);
	}

	void FileTreeModel::init()
	{
		connect_signals();
		reinit_view();
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
				page.childs.insert(leaf);
				++first;
			}
			else // PAGE
			{
				auto page_ptr = std::make_unique<page_type>();
				page_ptr->parent = &page;
				page_ptr->node.name = name;

				auto issub = std::bind(is_subelement, std::cref(prefix), std::cref(name), std::placeholders::_1);
				auto it = std::find_if_not(first, last, viewed::make_indirect_fun(std::move(issub)));
				fill_page(*page_ptr, std::move(newprefix), first, it);
				page.childs.insert(std::move(page_ptr));
				first = it;
			}
		}

		{	// now sort items in a page
			auto & seq_view = page.childs.get<by_seq>();

			std::vector<std::reference_wrapper<const value_ptr>> refs;
			refs.assign(seq_view.begin(), seq_view.end());

			auto refs_first = refs.begin();
			auto refs_last = refs.end();
			std::sort(refs_first, refs_last, m_sorter);

			seq_view.rearrange(refs_first);
		}		
	}

	void FileTreeModel::update_data(const signal_range_type & sorted_erased, const signal_range_type & sorted_updated, const signal_range_type & inserted)
	{
		return;

		QStringRef prefix;
		auto & page = m_root;

		processing_context ctx;
		int_vector affected_indexes;
		auto & container = page.childs;
		auto & seq_view  = container.get<by_seq>();
		auto & code_view = container.get<by_code>();

		for (;;)
		{
			std::uintptr_t type;
			QStringRef newprefix;
			QString erased_name, inserted_name, updated_name;

			while (ctx.erased_first != ctx.erased_last)
			{
				const auto & item = **ctx.erased_first;
				std::tie(type, erased_name, newprefix) = analyze(prefix, item);
				if (type == PAGE) break;

				auto it = container.find(item.filename);
				if (it != container.end())
				{
					auto seqit = container.project<by_seq>(it);
					auto pos = seqit - seq_view.begin();
					container.erase(it);
				}
					
			}

			while (ctx.inserted_first != ctx.inserted_last)
			{
				const auto * item = *ctx.inserted_first;
				std::tie(type, erased_name, newprefix) = analyze(prefix, *item);
				if (type == PAGE) break;

				bool inserted;
				std::tie(std::ignore, inserted) = container.insert(item);
				assert(inserted); (void)inserted;
			}

			while (ctx.updated_first != ctx.updated_last)
			{
				const auto * item = *ctx.inserted_first;
				std::tie(type, erased_name, newprefix) = analyze(prefix, *item);
				if (type == PAGE) break;

				auto it = container.find(item->filename);
				if (it != container.end())
					code_view.insert(item);
				else
				{
					auto seqit = container.project<by_seq>(it);
					auto pos = seqit - seq_view.begin();
				}
			}

			// at this point only pages are at front of ranges
			QString name = std::min({erased_name, updated_name, inserted_name});
			if (name.isEmpty()) break;

			auto issub = viewed::make_indirect_fun(std::bind(is_subelement, std::cref(prefix), std::cref(name), std::placeholders::_1));
			auto cur1 = std::find_if_not(ctx.inserted_first, ctx.inserted_last, issub);
			auto cur2 = std::find_if_not(ctx.updated_first,  ctx.updated_last,  issub);
			auto cur3 = std::find_if_not(ctx.erased_first,   ctx.erased_last,   issub);

			auto it = container.find(name);
			auto * newpage = reinterpret_cast<page_ptr>(it->ptr);
			newpage->parent = &page;
			newpage->node.name = name;

			//fill_page(*page_ptr, std::move(newprefix), ...);
			//container.insert(std::move(page_ptr));
		}
	}

	void FileTreeModel::erase_records(const signal_range_type & sorted_erased)
	{

	}

	void FileTreeModel::clear_view()
	{
		beginResetModel();

		m_root.childs.clear();		
		
		endResetModel();
	}

	FileTreeModel::FileTreeModel(std::shared_ptr<torrent_file_store> store, QObject * parent /* = nullptr */)
		: base_type(parent), m_owner(std::move(store))
	{
		m_root.parent = nullptr;
		init();
	}
}
