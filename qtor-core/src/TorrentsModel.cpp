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

	int TorrentsModel::FullRowCount() const
	{
		return qint(m_owner->size());
	}

	const torrent & TorrentsModel::GetItem(int row) const
	{
		return *m_store.at(row);
	}

	void TorrentsModel::FilterBy(QString expr)
	{
		filter_by(expr);
		//auto rtype = m_filter_pred.set_expr(FromQString<string_type>(expr));
		//return refilter_and_notify(rtype);
	}

	void TorrentsModel::SortBy(int column, Qt::SortOrder order)
	{
		return sort_by(m_columns[column], order == Qt::AscendingOrder);
		//m_sort_pred = sparse_container_comparator(m_columns[column], order == Qt::AscendingOrder);
		//view_type::sort_and_notify(m_store.begin(), m_store.end());
	}

	TorrentsModel::TorrentsModel(std::shared_ptr<torrent_store> store, QObject * parent)
		: base_type(parent), view_type(store.get())
	{
		assert(store);
		m_recstore = std::move(store);
		m_recstore->view_addref();

		m_filter_pred.set_items({torrent::Name});
		auto * meta = new torrent_meta(this);
		m_meta = meta;
		m_fmt  = meta;

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

		// from view_base_type
		connect_signals();
		reinit_view();
	}

	TorrentsModel::~TorrentsModel()
	{
		m_recstore->view_release();
	}
}
