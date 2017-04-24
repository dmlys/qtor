#pragma once
#include <qtor/torrent.hpp>
#include <qtor/torrent_filter.hpp>
#include <qtor/torrent_comparator.hpp>
#include <qtor/torrent_store.hpp>

#include <viewed/sfview_qtbase.hpp>

#include <QtCore/QString>
#include <QtCore/QAbstractItemModel>

namespace qtor
{
	class torrent_model : 
		public QAbstractTableModel,
		public viewed::sfview_qtbase
		<
			torrent_store,
			torrent_comparator,
			torrent_filter
		>
	{
		typedef torrent_model self_type;
		typedef QAbstractTableModel base_type;

		typedef viewed::sfview_qtbase
		<
			torrent_store,
			torrent_comparator,
			torrent_filter
		> view_type;

	private:
		//using view_type::m_owner;
		//using view_type::m_store;   // vector of pointers
		//using view_type::m_sort_pred;
		//using view_type::m_filter_pred;
		//using view_type::sort;

	private:
		std::shared_ptr<torrent_store> m_recstore;

	public:
		const torrent & get_torrent(int row) const;
		torrent_id_type get_torrent_id(int row) const;

		bool matches(const torrent & val) const noexcept;
		void filter_by(QString expr);
		void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

		int rowCount(const QModelIndex & parent = QModelIndex()) const override;
		int columnCount(const QModelIndex & parent = QModelIndex()) const override;

	public:
		torrent_model() = default;
		~torrent_model() = default;
		Q_DISABLE_COPY(torrent_model);
	};
}