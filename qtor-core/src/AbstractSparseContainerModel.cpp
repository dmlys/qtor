#include <QtCore/QSize>
#include <qtor/AbstractSparseContainerModel.hqt>

namespace qtor
{
	auto AbstractSparseContainerModel::GetItem(int row, int column) const 
		-> const sparse_container::any_type & 
	{
		auto & item = GetItem(row);
		return item.get_item(column);
	}

	int AbstractSparseContainerModel::columnCount(const QModelIndex & parent /* = QModelIndex() */) const
	{
		return qint(m_columns.size());
	}

	QString AbstractSparseContainerModel::FieldName(int section) const
	{
		if (section >= m_columns.size())
			return QString::null;

		unsigned column = m_columns[section];
		return m_formatter->item_name(column);
	}


	QString AbstractSparseContainerModel::GetValueShort(int row, int column) const
	{
		if (column >= m_columns.size())
			return QString::null;

		const auto & item = GetItem(row, column);
		return m_formatter->format_item_short(column, item);
	}

	QString AbstractSparseContainerModel::GetValue(int row, int column) const
	{
		if (column >= m_columns.size())
			return QString::null;

		const auto & item = GetItem(row, column);
		return m_formatter->format_item(column, item);
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
				return GetValueShort(row, column);

				//case TorrentRole: return QVariant::fromValue(GetTorrent(row));
			default:          return {};
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
}
