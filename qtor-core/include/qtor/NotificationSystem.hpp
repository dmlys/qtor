#pragma once
#include <memory>
#include <vector>

#include <viewed/sfview_qtbase.hpp>
#include <viewed/ptr_sequence_container.hpp>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QAbstractItemModel>
#include <QtWidgets/QAbstractItemDelegate>



namespace QtTools
{
	class NotificationPopupWidget;

	/// Simple notification system. More or less.
	class NotificationSystem : public QObject
	{
		Q_OBJECT

	public:
		class Notification;
		class Store;
		class Model;

		class TimestampPred;
		class NotificationFilter;
		class SimpleNotification;

		enum NotificationType
		{
			Info, Warn, Error
		};

	public:
		virtual std::unique_ptr<Model> CreateModel();

		virtual std::shared_ptr<Store>       GetStore();
		virtual std::shared_ptr<const Store> GetStore() const;

	public:
		void setParent(QWidget * parent) { QObject::setParent(parent); }
		auto parent() const -> QWidget * { return static_cast<QWidget *>(QObject::parent()); }

	public:
		NotificationSystem(QWidget * parent = nullptr);
		virtual ~NotificationSystem() = default;
	};

	class NotificationSystem::Notification : public QObject
	{
		Q_OBJECT

	public:
		virtual QDateTime Timestamp() const = 0;
		virtual QString Text() const = 0;
		virtual QAbstractItemDelegate * CreateDelegate() const = 0;
		virtual NotificationPopupWidget * CreatePopup() const = 0;

		virtual ~Notification() = default;
	};


	class NotificationSystem::SimpleNotification : public Notification
	{
	protected:
		QDateTime m_timestamp;
		QString m_title;
		QString m_text;
		QIcon m_icon;

	public:
		virtual QDateTime Timestamp() const override;
		virtual QString Text() const override;
		virtual QAbstractItemDelegate * CreateDelegate() const override;
		virtual NotificationPopupWidget * CreatePopup() const override;
	};


	class NotificationSystem::Store : 
		public viewed::ptr_sequence_container<Notification>
	{
		using self_type = Store;
		using base_type = viewed::ptr_sequence_container<Notification>;

	public:
		using base_type::base_type;
	};


	class NotificationSystem::TimestampPred
	{
	public:
		bool operator()(const Notification * n1, const Notification * n2) const
		{ 
			return operator()(*n1, *n2); 
		}

		bool operator()(const Notification & n1, const Notification & n2) const
		{
			return n1.Timestamp() < n2.Timestamp();
		}
	};


	class NotificationSystem::NotificationFilter
	{
	private:
		QString m_filter;

	public:
		// same, incremental
		viewed::refilter_type set_expr(QString search);

		bool matches(const Notification & n) const noexcept;
		bool always_matches() const noexcept;

		bool operator()(const Notification & n) const noexcept { return matches(n); }
		bool operator()(const Notification * n) const noexcept { return matches(*n); }
		explicit operator bool() const noexcept { return not always_matches(); }
	};

	class NotificationSystem::Model :
		public QAbstractListModel,
		public viewed::sfview_qtbase<Store, TimestampPred, NotificationFilter>
	{
		using self_type = Model;
		using view_type = viewed::sfview_qtbase<Store, TimestampPred, NotificationFilter>;
		using base_type = QAbstractListModel;

	private:
		using view_type::m_owner;
		using view_type::m_store;   // vector of pointers
		using view_type::m_sort_pred;
		using view_type::m_filter_pred;

	private:
		std::shared_ptr<Store> m_owner_store;

	public:
		virtual const Notification * GetItem(int row) const;

		virtual void FilterBy(QString expr);
		virtual void SortBy(int column, Qt::SortOrder order) ;
		virtual int FullRowCount() const;

		int rowCount(const QModelIndex & parent = QModelIndex()) const override;

	public:
		Model(std::shared_ptr<Store> store, QObject * parent = nullptr);
		~Model();

		Q_DISABLE_COPY(Model);

	};
}
