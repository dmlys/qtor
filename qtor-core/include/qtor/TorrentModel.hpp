#pragma once
#include <qtor/torrent.hpp>
#include <qtor/torrent_store.hpp>

#include <viewed/sfview_qtbase.hpp>
#include <qtor/AbstractSparseContainerModel.hqt>

#include <QtCore/QString>
#include <QtCore/QAbstractItemModel>

namespace qtor
{
	class TorrentModel : 
		public AbstractSparseContainerModel,
		public viewed::sfview_qtbase
		<
			torrent_store,
			sparse_container_comparator,
			sparse_container_filter
		>
	{
		using self_type = TorrentModel;
		using base_type = AbstractSparseContainerModel;

		typedef viewed::sfview_qtbase
		<
			torrent_store,
			sparse_container_comparator,
			sparse_container_filter
		> view_type;

	private:
		using view_type::m_owner;
		using view_type::m_store;   // vector of pointers
		using view_type::m_sort_pred;
		using view_type::m_filter_pred;

	private:
		std::shared_ptr<torrent_store> m_recstore;

	public:		
		const torrent & GetItem(int row) const override;

		void FilterBy(QString expr) override;
		int FullRowCount() const override;

		void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
		int rowCount(const QModelIndex & parent = QModelIndex()) const override;

	public:		
		TorrentModel(std::shared_ptr<torrent_store> store, QObject * parent = nullptr);
		~TorrentModel();

		Q_DISABLE_COPY(TorrentModel);
	};
}