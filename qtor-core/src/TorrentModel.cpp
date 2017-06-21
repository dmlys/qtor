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

	const torrent & TorrentModel::GetTorrent(int row) const
	{
		return *m_store.at(row);
	}

	void TorrentModel::FilterByName(QString expr)
	{
		auto search = FromQString(expr);

		auto rtype = m_filter_pred.set_expr(std::move(search));
		return refilter_and_notify(rtype);
	}

	void TorrentModel::sort(int column, Qt::SortOrder order)
	{
		m_sort_pred = create_comparator(column, order == Qt::AscendingOrder);
		view_type::sort_and_notify(m_store.begin(), m_store.end());
	}

	TorrentModel::TorrentModel(std::shared_ptr<torrent_store> store, QObject * parent)
		: base_type(parent), view_type(store.get())
	{
		assert(store);
		m_recstore = std::move(store);
		m_recstore->view_addref();

		// from view_base_type
		connect_signals();
		reinit_view();
	}

	TorrentModel::~TorrentModel()
	{
		m_recstore->view_release();
	}
}
