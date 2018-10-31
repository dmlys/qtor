#include <QtCore/QSize>
#include <qtor/AbstractSparseContainerModel.hqt>

namespace qtor
{
	int AbstractSparseContainerModel::FindColumn(unsigned id) const
	{
		auto first = m_columns.begin();
		auto last = m_columns.end();
		auto it = std::find(first, last, id);

		return it == last ? -1 : it - first;
	}

	auto AbstractSparseContainerModel::GetItem(int row, int column) const 
		-> const sparse_container::any_type & 
	{
		auto & item = GetItem(row);
		return item.get_item(m_columns[column]);
	}

	int AbstractSparseContainerModel::columnCount(const QModelIndex & parent /* = QModelIndex() */) const
	{
		return qint(m_columns.size());
	}

	QString AbstractSparseContainerModel::FieldName(int section) const
	{
		if (section >= qint(m_columns.size()))
			return QString::null;

		return m_meta->item_name(m_columns[section]);
	}


	QString AbstractSparseContainerModel::GetStringShort(int row, int column) const
	{
		if (column >= qint(m_columns.size()))
			return QString::null;

		column = m_columns[column];
		const auto & cont = GetItem(row);
		const auto & val = cont.get_item(column);
		return m_fmt->format_item_short(val, column);
	}

	QString AbstractSparseContainerModel::GetString(int row, int column) const
	{
		if (column >= qint(m_columns.size()))
			return QString::null;

		column = m_columns[column];
		const auto & cont = GetItem(row);
		const auto & val = cont.get_item(column);
		return m_fmt->format_item(val, column);
	}

	QVariant AbstractSparseContainerModel::data(const QModelIndex & index, int role /* = Qt::DisplayRole */) const
	{
		if (!index.isValid())
			return {};

		int row = index.row();
		int column = index.column();

		switch (role)
		{
			case Qt::DisplayRole:
			case Qt::ToolTipRole:
				return GetStringShort(row, column);

			case Qt::UserRole: return GetItem(index);
			default:           return {};
		}
	}

	QVariant AbstractSparseContainerModel::headerData(int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */) const
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

	void AbstractSparseContainerModel::SetColumns(std::vector<unsigned> columns)
	{
		beginResetModel();
		m_columns = std::move(columns);
		endResetModel();
	}

	void AbstractSparseContainerModel::SetFilter(QString expr)
	{
		m_filterStr = std::move(expr);
		FilterBy(m_filterStr);
		Q_EMIT FilterChanged(m_filterStr);
	}

	void AbstractSparseContainerModel::sort(int column, Qt::SortOrder order /* = Qt::AscendingOrder */)
	{
		m_sortColumn = column;
		m_sortOrder = order;

		SortBy(column, order);
		Q_EMIT SortingChanged(column, order);
	}
}
