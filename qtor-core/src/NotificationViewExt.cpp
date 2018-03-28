#include <qtor/NotificationSystem.hqt>
#include <qtor/NotificationSystemExt.hqt>
#include <qtor/NotificationView.hqt>
#include <qtor/NotificationViewExt.hqt>

namespace QtTools::NotificationSystem
{
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

	viewed::refilter_type NotificationFilter::set_expr(NotificationLevelBitset levels)
	{
		if (m_levels == levels) 
			return viewed::refilter_type::same;

		m_levels = std::move(levels);
		return viewed::refilter_type::full;
	}

	viewed::refilter_type NotificationFilter::set_expr(NotificationPriorityBitset priorities)
	{
		if (m_priorities == priorities)
			return viewed::refilter_type::same;

		m_priorities = std::move(priorities);
		return viewed::refilter_type::full;
	}

	bool NotificationFilter::matches(const Notification & n) const
	{
		if (not m_levels.test(n.Level()))
			return false;
		
		if (not m_priorities.test(n.Priority()))
			return false;

		if (n.Text().contains(m_filter, Qt::CaseInsensitive))
			return true;

		if (n.PlainText().contains(m_filter, Qt::CaseInsensitive))
			return true;

		return false;
	}

	bool NotificationFilter::always_matches() const noexcept
	{
		return m_filter.isEmpty() 
			and (m_levels.none() or m_levels.all())
			and (m_priorities.none() or m_priorities.all());
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

	auto NotificationModel::GetItem(int row) const -> const Notification &
	{
		return *m_store.at(row);
	}

	void NotificationModel::FilterBy(QString expr)
	{
		auto rtype = m_filter_pred.set_expr(expr);
		refilter_and_notify(rtype);
	}

	void NotificationModel::FilterBy(NotificationLevelBitset levels)
	{
		auto rtype = m_filter_pred.set_expr(levels);
		refilter_and_notify(rtype);
	}

	void NotificationModel::FilterBy(NotificationPriorityBitset priorities)
	{
		auto rtype = m_filter_pred.set_expr(priorities);
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
				return GetItem(row).Text();

			default:          return {};
		}
	}
}
