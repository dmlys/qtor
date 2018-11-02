#pragma once
#include <qtor/torrent.hpp>
#include <qtor/torrent_store.hpp>
#include <qtor/AbstractItemModel.hqt>

#include <viewed/sfview_qtbase.hpp>

namespace qtor
{
	class TorrentsModel :
	    public AbstractTableItemModel,
		public viewed::sfview_qtbase
		<
			torrent_store,
			sparse_container_comparator,
			sparse_container_filter
		>
	{
		using self_type = TorrentsModel;
		using base_type = AbstractTableItemModel;

		using view_type = viewed::sfview_qtbase
		<
			torrent_store,
			sparse_container_comparator,
			sparse_container_filter
		>;

	private:
		using view_type::m_owner;
		using view_type::m_store;   // vector of pointers
		using view_type::m_sort_pred;
		using view_type::m_filter_pred;

	private:
		std::shared_ptr<torrent_store> m_recstore;

	protected:
		virtual void SortBy(int column, Qt::SortOrder order) override;
		virtual void FilterBy(QString expr) override;

	public:
		//virtual Qt::ItemFlags flags(const QModelIndex & index) const override;
		virtual QVariant GetEntity(const QModelIndex & index) const override;
		virtual QVariant GetItem(const QModelIndex & index) const override;
		static QString StatusString(unsigned status);

	public:
		int FullRowCount(const QModelIndex & parent = QModelIndex()) const override;
		int rowCount(const QModelIndex & parent = QModelIndex()) const override;

	public:
		TorrentsModel(std::shared_ptr<torrent_store> store, QObject * parent = nullptr);
		~TorrentsModel();

		Q_DISABLE_COPY(TorrentsModel);
	};
}
