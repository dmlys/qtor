#include <ext/utility.hpp>
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
			this->~value_ptr();
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

	void FileTreeModel::fill_page(page_type & page, QStringRef prefix, std::vector<torrent_file>::const_iterator first, std::vector<torrent_file>::const_iterator last)
	{
		for (;;)
		{
			if (first == last) break;

			auto & path = first->filename;
			int pos = path.indexOf('/', prefix.length());
			int n   = pos - prefix.length();

			if (pos == -1) // LEAF
			{
				auto * leaf = &*first;
				page.childs.insert(leaf);
				++first;
			}
			else // PAGE
			{
				auto page_ptr = std::make_unique<page_type>();
				page_ptr->parent = &page;

				QString name = path.mid(prefix.length(), n);
				page_ptr->node.name = name;

				auto it = first;
				for (++it; it != last; ++it)
				{
					auto ref = it->filename.midRef(prefix.length(), n);
					if (ref != name) break;
				}					

				fill_page(*page_ptr, path.leftRef(pos + 1), first, it);
				page.childs.insert(std::move(page_ptr));
				first = it;
			}
		}
	}

	void FileTreeModel::Init(std::vector<torrent_file> & vals)
	{
		beginResetModel();

		m_root.parent = nullptr;
		m_root.childs.clear();

		auto first = vals.begin();
		auto last = vals.end();
		std::sort(first, last, [](auto & v1, auto & v2) { return v1.filename > v2.filename; });
		fill_page(m_root, {}, first, last);

		endResetModel();
	}

	FileTreeModel::FileTreeModel(QObject * parent /* = nullptr */)
		: base_type(parent)
	{
		m_root.parent = nullptr;
	}
}
