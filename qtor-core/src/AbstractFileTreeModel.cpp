#include <qtor/AbstractFileTreeModel.hqt>

namespace qtor
{
	void AbstractFileTreeModel::InitColumns()
	{
		m_columnNames.clear();
		m_columnNames 
			<< tr("fname")
			<< tr("total size")
			<< tr("have size")
			<< tr("wanted");
	}

	int AbstractFileTreeModel::FindColumn(unsigned id) const
	{
		auto first = m_columnNames.begin();
		auto last = m_columnNames.end();
		auto it = std::find(first, last, id);

		return it == last ? -1 : it - first;
	}

	QString AbstractFileTreeModel::FieldName(int section) const
	{
		if (section >= m_columnNames.size())
			return QString::null;

		return m_columnNames[section];
	}

	int AbstractFileTreeModel::columnCount(const QModelIndex & parent /* = QModelIndex() */) const
	{
		return torrent_file::FiledCount;
	}

	void AbstractFileTreeModel::SetFilter(QString expr)
	{
		FilterBy(m_filterStr = std::move(expr));
		Q_EMIT FilterChanged(m_filterStr);
	}

	void AbstractFileTreeModel::sort(int column, Qt::SortOrder order /* = Qt::AscendingOrder */)
	{
		m_sortColumn = column;
		m_sortOrder = order;

		SortBy(column, order);
		Q_EMIT SortingChanged(column, order);
	}

	QVariant AbstractFileTreeModel::headerData(int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */) const
	{
		if (orientation != Qt::Horizontal or role != Qt::DisplayRole)
			return {};

		return FieldName(section);
	}

	QVariant AbstractFileTreeModel::data(const QModelIndex & index, int role /* = Qt::DisplayRole */) const
	{
		if (not index.isValid())
			return {};

		if (role != Qt::DisplayRole and role != Qt::ToolTipRole)
			return {};

		return GetItem(index);
	}
}
