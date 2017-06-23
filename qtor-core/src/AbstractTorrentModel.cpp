#include <qtor/AbstractTorrentModel.hqt>

namespace qtor
{
	const QStringList AbstractTorrentModel::ms_columnNames =
	{
		QStringLiteral("Name"),
		QStringLiteral("TotalSize"),
		QStringLiteral("DownloadSpeed"),
		QStringLiteral("UploadSpeed"),
		QStringLiteral("DateAdded"),
		QStringLiteral("DateCreated"),
	};

	QVariant AbstractTorrentModel::GetItem(torrent & t, unsigned id)
	{
		switch (id)
		{
			case Name:           return QVariant::fromValue(t.name);
			case TotalSize:      return QVariant::fromValue(t.total_size);
			case DownloadSpeed:  return QVariant::fromValue(t.download_speed);
			case UploadSpeed:    return QVariant::fromValue(t.upload_speed);
			case DateAdded:      return QVariant::fromValue(t.date_added);
			case DateCreated:    return QVariant::fromValue(t.date_created);
			default:             return QVariant {};
		}
	}

	int AbstractTorrentModel::columnCount(const QModelIndex & parent /* = QModelIndex() */) const
	{
		return qint(m_columns.size());
	}

	QString AbstractTorrentModel::FieldName(int section) const
	{
		if (section >= m_columns.size())
			return QString::null;

		unsigned column = m_columns[section];
		return ms_columnNames[column];
	}

	QString AbstractTorrentModel::TorrentId(int row) const
	{
		return m_formatter->format_string(GetTorrent(row).id);
	}

	QString AbstractTorrentModel::GetValueShort(int row, int column) const
	{
		if (column >= m_columns.size())
			return QString::null;

		const torrent & t = GetTorrent(row);
		switch (m_columns[column])
		{
			case Name:           return m_formatter->format_short_string(t.name);
			case TotalSize:      return m_formatter->format_size(t.total_size);
			case DownloadSpeed:  return m_formatter->format_speed(t.download_speed);
			case UploadSpeed:    return m_formatter->format_speed(t.upload_speed);
			case DateAdded:      return m_formatter->format_datetime(t.date_added);
			case DateCreated:    return m_formatter->format_datetime(t.date_created);

			default: return QString::null;
		}
	}

	QString AbstractTorrentModel::GetValue(int row, int column) const
	{
		if (column >= m_columns.size())
			return QString::null;

		const torrent & t = GetTorrent(row);
		switch (m_columns[column])
		{
			case Name:           return m_formatter->format_string(t.name);
			case TotalSize:      return m_formatter->format_size(t.total_size);
			case DownloadSpeed:  return m_formatter->format_speed(t.download_speed);
			case UploadSpeed:    return m_formatter->format_speed(t.upload_speed);
			case DateAdded:      return m_formatter->format_datetime(t.date_added);
			case DateCreated:    return m_formatter->format_datetime(t.date_created);
			
			default: return QString::null;
		}
	}

	QVariant AbstractTorrentModel::data(const QModelIndex & index, int role /* = Qt::DisplayRole */) const
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

	QVariant AbstractTorrentModel::headerData(int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */) const
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

	void AbstractTorrentModel::SetColumns(std::vector<unsigned> columns)
	{
		beginResetModel();
		m_columns = std::move(columns);
		endResetModel();
	}

	AbstractTorrentModel::AbstractTorrentModel(QObject * parent /*= nullptr*/)
	{
		m_columns = 
		{
			Name,
			//TotalSize,
			//DownloadSpeed,
			//UploadSpeed,
			//DateAdded,
			//DateCreated,
		};
	}
}
