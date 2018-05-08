#include <ext/utility.hpp>
#include <QtTools/ToolsBase.hpp>
#include <qtor/FileTreeModel.hqt>

namespace qtor
{
	const FileTreeModel::value_vector FileTreeModel::ms_empty;

	inline FileTreeModel::value_ptr::value_ptr()
	{
		this->val = 0;
	}

	inline FileTreeModel::value_ptr::~value_ptr()
	{
		visit(delete_value);
	}

	inline FileTreeModel::value_ptr::value_ptr(value_ptr && op)
	{
		val = std::exchange(op.val, 0);
	}

	inline auto FileTreeModel::value_ptr::operator =(value_ptr && op) -> value_ptr &
	{
		if (this != &op)
		{
			this->~value_ptr();
			new(this) value_ptr(std::move(op));
		}

		return *this;
	}

	inline FileTreeModel::value_ptr::value_ptr(page_ptr ptr)
	{
		this->type = PAGE;
		this->ptr = reinterpret_cast<std::uintptr_t>(ptr);
	}

	inline FileTreeModel::value_ptr::value_ptr(leaf_ptr ptr)
	{
		this->type = LEAF;
		this->ptr = reinterpret_cast<std::uintptr_t>(ptr);
	}

	inline auto FileTreeModel::value_ptr::operator =(page_ptr ptr) -> value_ptr &
	{
		visit(delete_value);

		this->type = PAGE;
		this->ptr = reinterpret_cast<std::uintptr_t>(ptr);

		return *this;
	}

	inline auto FileTreeModel::value_ptr::operator =(leaf_ptr ptr) -> value_ptr &
	{
		visit(delete_value);

		this->type = LEAF;
		this->ptr = reinterpret_cast<std::uintptr_t>(ptr);

		return *this;
	}

	auto FileTreeModel::parent_page(page_ptr * ptr) const -> page_ptr *
	{
		return (*ptr)->parent;
	}

	auto FileTreeModel::parent_row(page_ptr * ptr) const -> int
	{
		page_ptr * ppage = parent_page(ptr);
		auto & gp_children = ppage ? (*ppage)->childs : m_elements;

		value_ptr * valptr = reinterpret_cast<value_ptr *>(ptr);
		return valptr - gp_children.data();
	}

	inline auto FileTreeModel::get_ppage(const QModelIndex & index) const -> page_ptr *
	{
		return static_cast<page_ptr *>(index.internalPointer());
	}

	auto FileTreeModel::get_element_ptr(const QModelIndex & index) const -> const value_ptr &
	{
		assert(index.isValid());		
		auto * ppage = get_ppage(index);
		const value_vector & children = ppage ? (*ppage)->childs : m_elements;

		assert(index.row() < children.size());
		return children[index.row()];
	}

	QString FileTreeModel::get_name(leaf_ptr ptr)
	{
		int pos = ptr->path.lastIndexOf('/') + 1;
		return ptr->path.mid(pos);
	}

	QString FileTreeModel::get_name(page_ptr ptr)
	{
		return ptr->node.name;
	}

	int FileTreeModel::columnCount(const QModelIndex & parent /* = QModelIndex() */) const
	{
		return 1;
	}

	int FileTreeModel::rowCount(const QModelIndex & parent /* = QModelIndex() */) const
	{
		if (not parent.isValid())
			return qint(m_elements.size());

		const auto & val = get_element_ptr(parent);
		const auto & children = get_children(val);
		return qint(children.size());
	}

	QModelIndex FileTreeModel::parent(const QModelIndex & index) const
	{
		if (not index.isValid()) return {};
		
		page_ptr * ppage = get_ppage(index);
		if (not ppage) return {};

		int row = parent_row(ppage);
		return createIndex(row, 0, parent_page(ppage));
	}

	QModelIndex FileTreeModel::index(int row, int column, const QModelIndex & parent /* = QModelIndex() */) const
	{
		const value_vector * children;
		page_ptr * ppage;

		if (not parent.isValid())
		{
			children = &m_elements;
			ppage = nullptr;
		}
		else
		{
			auto * page = get_ppage(parent);
			children = page ? &(*page)->childs : &m_elements;
			value_ptr * item = ext::unconst(children->data() + parent.row());
			ppage = reinterpret_cast<page_ptr *>(item);
		}

		if (children->size() < row)
			return {};

		return createIndex(row, column, ppage);
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

	void FileTreeModel::fill_page(value_vector & pages, QStringRef prefix, std::vector<file_element>::const_iterator first, std::vector<file_element>::const_iterator last)
	{
		for (;;)
		{
			if (first == last) break;

			auto & path = first->path;
			int pos = path.indexOf('/', prefix.length());
			int n   = pos - prefix.length();

			if (pos == -1) // LEAF
			{
				pages.push_back(&*first);
				++first;
			}
			else // PAGE
			{
				auto page_ptr = std::make_unique<page_type>();
				QString name = path.mid(prefix.length(), n);
				page_ptr->node.name = name;

				auto it = first;
				for (++it; it != last; ++it)
				{
					auto ref = it->path.midRef(prefix.length(), n);
					if (ref != name) break;
				}					

				fill_page(page_ptr->childs, path.leftRef(pos + 1), first, it);
				pages.push_back(page_ptr.release());
				first = it;
			}
		}
	}

	void FileTreeModel::set_parent(page_ptr * parent, value_vector & pages)
	{
		auto first = pages.data();
		auto last = first + pages.size();

		for (; first != last; ++first)
		{
			if (first->type == LEAF)
				continue;

			page_ptr ptr = reinterpret_cast<page_ptr>(first->ptr);
			ptr->parent = parent;

			set_parent(reinterpret_cast<page_ptr *>(first), ptr->childs);
		}
	}

	void FileTreeModel::Init(std::vector<file_element> & vals)
	{
		beginResetModel();

		auto first = vals.begin();
		auto last = vals.end();
		std::sort(first, last, [](auto & v1, auto & v2) { return v1.path > v2.path; });
		fill_page(m_elements, {}, first, last);
		set_parent(nullptr, m_elements);

		endResetModel();
	}
}
