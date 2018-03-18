#include <qtor/NotificationSystem.hqt>
#include <qtor/NotificationSystemExt.hqt>

#include <QtCore/QStringBuilder>
#include <QtCore/QThread>
#include <QtGui/QTextDocument>
#include <QtTools/ToolsBase.hpp>

namespace QtTools::NotificationSystem
{
	SimpleNotification::SimpleNotification()
		: m_priority(Normal), m_level(Info), m_priority_inited(0), m_level_inited(0)
	{

	}

	SimpleNotification::SimpleNotification(QString title, QString text, Qt::TextFormat textFmt,  QDateTime timestamp)
		: m_title(std::move(title)), m_text(std::move(text)), m_textFmt(textFmt), m_timestamp(std::move(timestamp)),
		  m_priority(Normal), m_level(Info), m_priority_inited(0), m_level_inited(0)
	{

	}

	static QString toPlain(QString rich)
	{
		QTextDocument doc;
		doc.setHtml(rich);

		return doc.toPlainText();
	}

	QString SimpleNotification::PlainText() const
	{
		switch (m_textFmt)
		{
			case Qt::PlainText: return m_text;				
			case Qt::RichText:  return toPlain(m_text);
				
			case Qt::AutoText:
			default:
				if (Qt::mightBeRichText(m_text))
					return toPlain(m_text);
				else
					return m_text;				
		}
	}

	auto SimpleNotification::Priority() const -> NotificationPriority
	{
		return static_cast<NotificationPriority>(m_priority);
	}

	auto SimpleNotification::Priority(NotificationPriority priority) -> NotificationPriority
	{
		auto ret = m_priority;
		m_priority = priority;
		m_priority_inited = 1;

		return static_cast<NotificationPriority>(ret);
	}

	auto SimpleNotification::Level() const -> NotificationLevel
	{
		return static_cast<NotificationLevel>(m_level);
	}

	auto SimpleNotification::Level(NotificationLevel level) -> NotificationLevel
	{
		auto ret = m_level;
		m_level = level;
		m_level_inited = 1;

		if (level == Error and not m_priority_inited)
			m_priority = High;

		return static_cast<NotificationLevel>(ret);
	}

	NotificationCenter::NotificationCenter(QObject * parent /* = nullptr */)
		: QObject(parent)
	{
		m_store = std::make_shared<NotificationStore>(this);
	}

	auto NotificationCenter::GetStore() -> std::shared_ptr<NotificationStore>
	{
		return m_store;
	}

	auto NotificationCenter::GetStore() const -> std::shared_ptr<const NotificationStore>
	{
		return m_store;
	}

	void NotificationCenter::AddNotification(QString title, QString text,
		Qt::TextFormat fmt /* = Qt::AutoText */, QDateTime timestamp /* = QDateTime::currentDateTime() */)
	{
		auto notification = std::make_unique<SimpleNotification>(std::move(title), std::move(text), fmt, std::move(timestamp));
		notification->TextFmt(fmt);

		AddNotification(std::move(notification));
	}

	void NotificationCenter::DoAddNotification(const Notification * notification)
	{
		m_store->push_back(notification);
		Q_EMIT NotificationAdded(m_store->back());
	}

	void NotificationCenter::AddNotification(std::unique_ptr<const Notification> notification)
	{		
		auto * thr = this->thread();
		if (thr and thr == thr->currentThread())
		{
			return DoAddNotification(notification.release());
		}
		else
		{
			QMetaObject::invokeMethod(this, "DoAddNotification", Qt::QueuedConnection,
				Q_ARG(const Notification *, notification.get()));

			notification.release();
		}
	}

	auto NotificationCenter::CreateNotification() const
		-> std::unique_ptr<Notification>
	{
		return std::make_unique<SimpleNotification>();
	}
}
