#include <qtor/AbstractItemModel.hqt>

namespace qtor
{
	/************************************************************************/
	/*                       AbstractItemModel                              */
	/************************************************************************/
	unsigned AbstractItemModel::MetaToViewIndex(unsigned meta_index) const
	{
		auto first = m_columns.begin();
		auto last  = m_columns.end();
		auto it = std::find(first, last, meta_index);

		return it == last ? -1 : it - first;
	}

	void AbstractItemModel::SetColumns(std::vector<unsigned> columns)
	{
		beginResetModel();
		m_columns = std::move(columns);
		endResetModel();
	}

	int AbstractItemModel::columnCount(const QModelIndex & parent /* = QModelIndex() */) const
	{
		return qint(m_columns.size());
	}

	QString AbstractItemModel::FieldName(int section) const
	{
		if (section >= qint(m_columns.size()))
			return QString::null;

		return m_meta->item_name(ViewToMetaIndex(section));
	}

	QString AbstractItemModel::FieldName(const QModelIndex & index) const
	{
		if (not index.isValid()) return QString::null;

		return FieldName(index.column());
	}

	QString AbstractItemModel::GetString(const QModelIndex & index) const
	{
		if (not index.isValid()) return QString::null;

		auto item = GetItem(index);
		auto meta_index = ViewToMetaIndex(index.column());
		auto type = m_meta->item_type(meta_index);

		return m_fmt->format_item(item, type);
	}

	QString AbstractItemModel::GetStringShort(const QModelIndex & index) const
	{
		if (not index.isValid()) return QString::null;

		auto item = GetItem(index);
		auto meta_index = ViewToMetaIndex(index.column());
		auto type = m_meta->item_type(meta_index);

		return m_fmt->format_item_short(item, type);
	}

	void AbstractItemModel::SetFilter(QString expr)
	{
		m_filterStr = std::move(expr);
		FilterBy(m_filterStr);
		Q_EMIT FilterChanged(m_filterStr);
	}

	void AbstractItemModel::sort(int column, Qt::SortOrder order /* = Qt::AscendingOrder */)
	{
		m_sortColumn = column;
		m_sortOrder = order;

		SortBy(column, order);
		Q_EMIT SortingChanged(column, order);
	}


	QVariant AbstractItemModel::data(const QModelIndex & index, int role /* = Qt::DisplayRole */) const
	{
		switch (role)
		{
		    case Qt::DisplayRole:
		    case Qt::ToolTipRole:
		    case Qt::UserRole: return GetItem(index);
		    default:           return {};
		}
	}

	QVariant AbstractItemModel::headerData(int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */) const
	{
		if (orientation == Qt::Vertical)
			return base_type::headerData(section, orientation, role);

		switch (role)
		{
		    case Qt::DisplayRole:
		    case Qt::ToolTipRole:
			    return FieldName(section);

		    default: return {};
		}
	}

	/************************************************************************/
	/*                      AbstractTableItemModel                          */
	/************************************************************************/
	Qt::ItemFlags AbstractTableItemModel::flags(const QModelIndex & index) const
	{
		Qt::ItemFlags flags = QAbstractItemModel::flags(index);
		//if (index.isValid())
		flags |= Qt::ItemNeverHasChildren;
		return flags;
	}

	QModelIndex AbstractTableItemModel::index(int row, int column, const QModelIndex & parent /*= QModelIndex()*/) const
	{
		return hasIndex(row, column, parent) ? createIndex(row, column) : QModelIndex();
	}

	QModelIndex AbstractTableItemModel::parent(const QModelIndex & child) const
	{
		return QModelIndex();
	}

	QModelIndex AbstractTableItemModel::sibling(int row, int column, const QModelIndex & idx) const
	{
		return index(row, column);
	}

	bool AbstractTableItemModel::hasChildren(const QModelIndex & parent) const
	{
		//return (parent.model() == this and not parent.isValid()) and (rowCount(parent) > 0 and columnCount(parent) > 0);
		return false;
	}
}
