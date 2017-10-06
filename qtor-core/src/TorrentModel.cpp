#include <qtor/TorrentModel.hpp>
#include <QtTools/ToolsBase.hpp>

namespace qtor
{
	int TorrentModel::rowCount(const QModelIndex & parent /* = QModelIndex() */) const
	{
		return qint(m_store.size());
	}

	int TorrentModel::FullRowCount() const
	{
		return qint(m_owner->size());
	}

	const torrent & TorrentModel::GetItem(int row) const
	{
		return *m_store.at(row);
	}

	void TorrentModel::FilterBy(QString expr)
	{
		auto search = FromQString(expr);

		auto rtype = m_filter_pred.set_expr(std::move(search));
		return refilter_and_notify(rtype);
	}

	void TorrentModel::sort(int column, Qt::SortOrder order)
	{
		m_sort_pred = sparse_container_comparator(m_columns[column], order == Qt::AscendingOrder);
		view_type::sort_and_notify(m_store.begin(), m_store.end());
	}

	TorrentModel::TorrentModel(std::shared_ptr<torrent_store> store, QObject * parent)
		: base_type(parent), view_type(store.get())
	{
		assert(store);
		m_recstore = std::move(store);
		m_recstore->view_addref();

		m_filter_pred.set_items({torrent::Name});
		m_meta = new torrent_meta(this);
		SetColumns({
			//torrent::Id,
			torrent::Name,
			//torrent::Creator,
			//torrent::Comment,

			torrent::ErrorString,
			torrent::Finished,
			torrent::Stalled,

			torrent::TotalSize,
			torrent::SizeWhenDone,

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

	TorrentModel::~TorrentModel()
	{
		m_recstore->view_release();
	}
}
