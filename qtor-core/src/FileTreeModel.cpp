#include <ext/utility.hpp>
#include <QtTools/ToolsBase.hpp>
#include <qtor/FileTreeModel.hqt>

namespace qtor
{
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


	template <class visitor>
	inline auto FileTreeModel::visit(value_ptr val, visitor && vis) -> std::invoke_result_t<visitor &&, leaf_ptr>
	{
		return val.type
			? std::forward<visitor>(vis)(reinterpret_cast<leaf_ptr>(val.ptr))
			: std::forward<visitor>(vis)(reinterpret_cast<page_ptr>(val.ptr));
	}

	inline auto FileTreeModel::get_ppage(const QModelIndex & index) const -> page_ptr *
	{
		return static_cast<page_ptr *>(index.internalPointer());
	}

	auto FileTreeModel::get_element(const QModelIndex & index) const -> value_ptr
	{
		assert(index.isValid());
		auto & page = **get_ppage(index);

		assert(index.row() < page.childs.size());
		return page.childs[index.row()];
	}

	int FileTreeModel::columnCount(const QModelIndex & parent /* = QModelIndex() */) const
	{
		return 1;
	}

	int FileTreeModel::rowCount(const QModelIndex & parent /* = QModelIndex() */) const
	{
		if (not parent.isValid())
			return qint(m_elements.size());

		auto val = get_element(parent);
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
		if (not parent.isValid())
			children = &m_elements;
		else
		{
			auto * page = get_ppage(parent);
			children = page ? &(*page)->childs : &m_elements;
		}

		if (children->size() >= row)
			return {};

		value_ptr * item = ext::unconst(children->data() + row);
		page_ptr  * ppage = reinterpret_cast<page_ptr *>(item);
		return createIndex(row, column, ppage);
	}

	QVariant FileTreeModel::data(const QModelIndex & index, int role /* = Qt::DisplayRole */) const
	{
		if (not index.isValid()) return {};

		auto valptr = get_element(index);
		switch (role)
		{
			case Qt::DisplayRole:
			case Qt::ToolTipRole:
				//return page.val->index();

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

	}

	//void FileTreeModel::fill_pages(page_vector & pages, QStringRef prefix, std::vector<file_element>::const_iterator first, std::vector<file_element>::const_iterator last)
	//{
	//	if (first == last) return;
	//	auto & val = *first;

	//	for (;;)
	//	{
	//		if (first == last) break;

	//		page_type page;

	//		auto & path = first->path;
	//		int pos = path.indexOf('/', prefix.length());
	//		if (pos == -1)
	//		{
	//			page.val = &*first;
	//			pages.push_back(std::move(page));
	//			continue;
	//		}

	//		QStringRef ref = path.midRef(prefix.length(), pos);
	//		auto it = first;
	//		for (++it; first != last; ++it);

	//		fill_pages(page.childs, path.leftRef(pos), first, it);
	//		pages.push_back(std::move(page));
	//		first = it;
	//	}
	//}

	//void FileTreeModel::Init(std::vector<file_element> vals)
	//{

	//}
}
