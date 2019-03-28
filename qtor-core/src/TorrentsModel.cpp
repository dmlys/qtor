#include <qtor/TorrentsModel.hpp>
#include <QtTools/ToolsBase.hpp>

namespace qtor
{
	QString TorrentsModel::StatusString(unsigned status)
	{
		switch (status)
		{
			case torrent_status::unknown:            return tr("unknown");
			case torrent_status::stopped:            return tr("stopped");
			case torrent_status::downloading:        return tr("downloading");
			case torrent_status::seeding:            return tr("seeding");
			case torrent_status::checking:           return tr("checking");
			case torrent_status::downloading_queued: return tr("downloading_queued");
			case torrent_status::seeding_queued:     return tr("seeding_queued");
			case torrent_status::checking_queued:    return tr("checking_queued");

			default: return tr("unknown torrent status");
		}
	}

	int TorrentsModel::rowCount(const QModelIndex & parent /* = QModelIndex() */) const
	{
		return qint(m_store.size());
	}

	int TorrentsModel::FullRowCount(const QModelIndex & parent /* = QModelIndex() */) const
	{
		return qint(m_owner->size());
	}

	QVariant TorrentsModel::GetEntity(const QModelIndex & idx) const
	{
		if (not idx.isValid()) return QVariant();

		auto * item = m_store[idx.row()];
		return QVariant::fromValue(item);
	}

	QVariant TorrentsModel::GetItem(const QModelIndex & idx) const
	{
		if (not idx.isValid()) return QVariant();

		auto meta_index = ViewToMetaIndex(idx.column());
		auto * item = m_store[idx.row()];
		return item->get_item(meta_index);
	}

	void TorrentsModel::FilterBy(QString expr)
	{
		filter_by(expr);
	}

	void TorrentsModel::SortBy(int column, Qt::SortOrder order)
	{
		return sort_by(m_columns[column], order == Qt::AscendingOrder);
	}

	TorrentsModel::TorrentsModel(std::shared_ptr<torrent_store> store, QObject * parent)
		: base_type(parent), view_type(std::move(store))
	{
		assert(m_owner);
		m_owner->view_addref();

		m_filter_pred.set_items({torrent::Name});
		m_meta = std::make_shared<torrent_meta>();
		m_fmt  = std::make_shared<formatter>();

		SetColumns({
			//torrent::Id,
			torrent::Name,
			//torrent::Creator,
			//torrent::Comment,

			torrent::ErrorString,
			torrent::Finished,
			torrent::Stalled,

			torrent::CurrentSize,
			torrent::RequestedSize,
			torrent::TotalSize,

			torrent::DownloadSpeed,
			torrent::UploadSpeed,

			torrent::Eta,
			torrent::EtaIdle,

			torrent::DateAdded,
			torrent::DateCreated,
			torrent::DateStarted,
			torrent::DateDone,
		});
	}

	TorrentsModel::~TorrentsModel()
	{
		m_owner->view_release();
	}
}
