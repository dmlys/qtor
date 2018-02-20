#include <QtCore/QStringBuilder>
#include <QtTools/ToolsBase.hpp>
#include <qtor/NotificationSystem.hqt>
#include <qtor/NotificationSystemExt.hqt>

namespace QtTools::NotificationSystem
{
	SimpleNotification::SimpleNotification()
		: m_priority(Normal), m_level(Info), m_priority_inited(0), m_level_inited(0)
	{

	}

	SimpleNotification::SimpleNotification(QString title, QString text, QDateTime timestamp)
		: m_title(std::move(title)), m_text(std::move(text)), m_timestamp(std::move(timestamp)),
		  m_priority(Normal), m_level(Info), m_priority_inited(0), m_level_inited(0)
	{

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

	QString SimpleNotification::FilterText() const
	{
		QTextDocument doc;
		doc.setHtml(m_text);

		return m_title
			% QStringLiteral("\n")
			% doc.toPlainText();
	}

	QString SimpleNotification::ClipboardText() const
	{
		QTextDocument doc;
		doc.setHtml(m_text);

		return m_title % "  "
			% m_timestamp.toString(Qt::DateFormat::DefaultLocaleShortDate)
			% QStringLiteral("\n")
			% doc.toPlainText();
	}

	viewed::refilter_type NotificationFilter::set_expr(QString search)
	{
		if (search.compare(m_filter, Qt::CaseInsensitive) == 0)
			return viewed::refilter_type::same;
		else if (search.startsWith(m_filter, Qt::CaseInsensitive))
		{
			m_filter = std::move(search);
			return viewed::refilter_type::incremental;
		}
		else
		{
			m_filter = std::move(search);
			return viewed::refilter_type::full;
		}
	}

	bool NotificationFilter::matches(const Notification & n) const noexcept
	{
		return n.FilterText().contains(m_filter, Qt::CaseInsensitive);		
	}

	bool NotificationFilter::always_matches() const noexcept
	{
		return m_filter.isEmpty();
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

	void AbstractNotificationModel::SetFilter(QString expr)
	{
		m_filterStr = std::move(expr);
		FilterBy(m_filterStr);
		Q_EMIT FilterChanged(m_filterStr);
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

	auto NotificationCenter::CreateModel() -> std::shared_ptr<AbstractNotificationModel>
	{
		return std::make_shared<NotificationModel>(m_store);
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
