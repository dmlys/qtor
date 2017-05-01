#include <qtor/AbstractTorrentModel.hqt>
#include <QtTools/ToolsBase.hpp>
#include <QtTools/DateUtils.hpp>

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

	int AbstractTorrentModel::columnCount(const QModelIndex & parent /* = QModelIndex() */) const
	{
		return ms_columnNames.size();
	}

	QString AbstractTorrentModel::FieldName(int section) const
	{
		return section < ms_columnNames.size()
			? ms_columnNames[section]
			: QString::null;
	}

	QString AbstractTorrentModel::TorrentId(int row) const
	{
		return ToQString(GetTorrent(row).id);
	}

	QString AbstractTorrentModel::GetValueShort(int row, int column) const
	{
		return GetValue(row, column);
	}

	QString AbstractTorrentModel::GetValue(int row, int column) const
	{
		const torrent & t = GetTorrent(row);
		switch (column)
		{
			case torrent::Name:           return ToQString(t.name);
			case torrent::TotalSize:      return QString::number(t.total_size);
			case torrent::DownloadSpeed:  return QString::number(t.download_speed);
			case torrent::UploadSpeed:    return QString::number(t.upload_speed);
			case torrent::DateAdded:      return QtTools::ToQDateTime(t.date_added).toString();
			case torrent::DateCreated:    return QtTools::ToQDateTime(t.date_created).toString();
			
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
}
