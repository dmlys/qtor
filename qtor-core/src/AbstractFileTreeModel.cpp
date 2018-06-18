#include <qtor/AbstractFileTreeModel.hqt>

namespace qtor
{
	void AbstractFileTreeModel::InitColumns()
	{
		m_columnMeta = 
		{
			{torrent_file::FileName,  {QStringLiteral("fname"), 400} },
			{torrent_file::TotalSize, {QStringLiteral("fname"), 400} },
			{torrent_file::HaveSize,  {QStringLiteral("fname"), 400} },
			{torrent_file::Index,     {QStringLiteral("fname"), 400} },
			{torrent_file::Priority,  {QStringLiteral("fname"), 400} },
			{torrent_file::Wanted,    {QStringLiteral("fname"), 400} },
		};

		m_columns.assign({
			torrent_file::FileName,
			torrent_file::TotalSize,
			torrent_file::HaveSize
		});
	}

	int AbstractFileTreeModel::FindColumn(unsigned id) const
	{
		auto first = m_columns.begin();
		auto last = m_columns.end();
		auto it = std::find(first, last, id);

		return it == last ? -1 : it - first;
	}

	QString AbstractFileTreeModel::FieldName(int section) const
	{
		if (section >= m_columns.size())
			return QString::null;

		return m_columnMeta.at(m_columns[section]).fieldName;
	}

	int AbstractFileTreeModel::ColumnSizeHint(int section) const
	{
		if (section >= m_columns.size())
			return -1;

		return m_columnMeta.at(m_columns[section]).columnSizeHint;
	}

	int AbstractFileTreeModel::columnCount(const QModelIndex & parent /* = QModelIndex() */) const
	{
		return qint(m_columns.size());
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
		if (orientation != Qt::Horizontal)
			return {};

		switch (role)
		{
			case Qt::DisplayRole:
			case Qt::ToolTipRole:
				return FieldName(section);

			default: return {};
		}
	}

	QVariant AbstractFileTreeModel::data(const QModelIndex & index, int role /* = Qt::DisplayRole */) const
	{
		if (not index.isValid())
			return {};

		switch (role)
		{
			case Qt::DisplayRole:
			case Qt::ToolTipRole:
				return GetItem(index);

			//case Qt::SizeHintRole:
			//	return QSize {ColumnSizeHint(index.column()), -1};

			default: return {};
		}
	}
}
