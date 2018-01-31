#include <QtTools/ToolsBase.hpp>
#include <qtor/NotificationSystem.hqt>
#include <qtor/NotificationSystemExt.hqt>

namespace QtTools::NotificationSystem
{
	viewed::refilter_type NotificationFilter::set_expr(QString search)
	{
		return viewed::refilter_type::same;
	}

	bool NotificationFilter::matches(const Notification & n) const noexcept
	{
		return true;
	}

	bool NotificationFilter::always_matches() const noexcept
	{
		return true;
	}

	NotificationModel::NotificationModel(std::shared_ptr<NotificationStore> store, QObject * parent /* = nullptr */)
		: view_type(store.get()), base_type(parent)
	{
		assert(store);

		m_owner_store = std::move(store);

		// from view_base_type
		connect_signals();
		reinit_view();
	}

	auto NotificationModel::GetNotificationCenter() const -> QPointer<NotificationCenter>
	{
		return m_owner->GetNotificationCenter();
	}

	auto NotificationModel::GetItem(int row) const -> const Notification *
	{
		return m_store.at(row);
	}

	void NotificationModel::FilterBy(QString expr)
	{
		auto rtype = m_filter_pred.set_expr(expr);
		refilter_and_notify(rtype);
	}

	int NotificationModel::FullRowCount() const
	{
		return qint(m_owner_store->size());
	}

	int NotificationModel::rowCount(const QModelIndex & parent /*= QModelIndex()*/) const
	{
		return qint(m_store.size());
	}

	QVariant AbstractNotificationModel::data(const QModelIndex & index, int role /*= Qt::DisplayRole*/) const
	{
		if (!index.isValid())
			return {};

		int row = index.row();
		int column = index.column();

		switch (role)
		{
			//case Qt::UserRole:
			//	return QVariant::fromValue(GetItem(row));

			case Qt::DisplayRole:
			case Qt::ToolTipRole:
				return GetItem(row)->Text();

			default:          return {};
		}
	}

	NotificationCenter::NotificationCenter(QWidget * parent /* = nullptr */)
		: QObject(parent)
	{
		m_store = std::make_shared<NotificationStore>(this);
	}

	auto NotificationCenter::CreateModel() -> std::unique_ptr<AbstractNotificationModel>
	{
		return std::make_unique<NotificationModel>(m_store);
	}

	auto NotificationCenter::GetStore() -> std::shared_ptr<NotificationStore>
	{
		return m_store;
	}

	auto NotificationCenter::GetStore() const -> std::shared_ptr<const NotificationStore>
	{
		return m_store;
	}

	void NotificationCenter::AddNotification(QString title, QString text, QDateTime timestamp /* = QDateTime::currentDateTime() */)
	{
		auto notification = std::make_unique<SimpleNotification>(std::move(title), std::move(text), std::move(timestamp));
		AddNotification(std::move(notification));
	}

	void NotificationCenter::AddNotification(std::unique_ptr<const Notification> notification)
	{
		m_store->push_back(notification.release());
	}
}
