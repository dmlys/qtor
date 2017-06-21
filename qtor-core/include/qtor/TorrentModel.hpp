#pragma once
#include <qtor/torrent.hpp>
#include <qtor/torrent_filter.hpp>
#include <qtor/torrent_comparator.hpp>
#include <qtor/torrent_store.hpp>

#include <viewed/sfview_qtbase.hpp>
#include <qtor/AbstractTorrentModel.hqt>

#include <QtCore/QString>
#include <QtCore/QAbstractItemModel>

namespace qtor
{
	class TorrentModel : 
		public AbstractTorrentModel,
		public viewed::sfview_qtbase
		<
			torrent_store,
			torrent_comparator,
			torrent_filter
		>
	{
		typedef TorrentModel          self_type;
		typedef AbstractTorrentModel  base_type;

		typedef viewed::sfview_qtbase
		<
			torrent_store,
			torrent_comparator,
			torrent_filter
		> view_type;

	private:
		using view_type::m_owner;
		using view_type::m_store;   // vector of pointers
		using view_type::m_sort_pred;
		using view_type::m_filter_pred;

	private:
		std::shared_ptr<torrent_store> m_recstore;

	public:		
		const torrent & GetTorrent(int row) const override;

		void FilterByName(QString expr) override;
		int FullRowCount() const override;

		void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
		int rowCount(const QModelIndex & parent = QModelIndex()) const override;

	public:		
		TorrentModel(std::shared_ptr<torrent_store> store, QObject * parent = nullptr);
		~TorrentModel();
		Q_DISABLE_COPY(TorrentModel);
	};
}