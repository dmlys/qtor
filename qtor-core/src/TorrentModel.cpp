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

		bool same, incremental;
		std::tie(same, incremental) = m_filter_pred.set_expr(std::move(search));
		if (same) return;

		beginResetModel();

		if (incremental)
			view_type::remove_filtered(m_store);
		else
			view_type::reinit_view();

		endResetModel();
	}

	void TorrentModel::sort(int column, Qt::SortOrder order)
	{
		m_sort_pred = create_comparator(column, order == Qt::AscendingOrder);
		view_type::sort_and_notify(m_store.begin(), m_store.end());
	}

	TorrentModel::TorrentModel(torrent_store & owner, QObject * parent)
		: base_type(parent), view_type(&owner)
	{

	}
}
