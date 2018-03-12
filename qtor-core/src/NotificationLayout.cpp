#include <qtor/NotificationSystem.hqt>
#include <qtor/NotificationSystemExt.hqt>
#include <qtor/NotificationLayout.hqt>

#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>

namespace QtTools::NotificationSystem
{
	NotificationLayout::~NotificationLayout()
	{
		for (auto & item : m_items)
		{
			// item.notification - we do not control it's lifetime;
			delete item.animation.data();
			delete item.widget.data();
		}
	}

	unsigned NotificationLayout::NotificationsCount() const
	{
		return static_cast<unsigned>(m_items.size());
	}

	auto NotificationLayout::NotificationAt(unsigned index) -> QPointer<const Notification>
	{
		if (index >= m_items.size())
			return {};

		return m_items[index].notification;
	}

	auto NotificationLayout::TakeNotification(unsigned index) -> QPointer<const Notification>
	{
		if (index >= m_items.size())
			return {};

		auto item = std::move(m_items[index]);
		m_items.erase(m_items.begin() + index);

		item.widget->close();
		delete item.widget.data();
		delete item.animation.data();

		ScheduleUpdate();
		return item.notification;
	}

	void NotificationLayout::AddNotification(QPointer<const Notification> notification, NotificationPopupWidget * widget)
	{
		Item item;
		item.notification = std::move(notification);
		item.widget = widget;

		m_items.push_back(std::move(item));		
		ScheduleUpdate();

		widget->setAttribute(Qt::WA_DeleteOnClose, true);
		connect(widget, &QObject::destroyed, this, &NotificationLayout::NotificationClosed);
	}

	void NotificationLayout::AddNotification(QPointer<const Notification> notification)
	{
		auto * popup = notification->CreatePopup();
		AddNotification(std::move(notification), std::move(popup));
	}


	void NotificationLayout::NotificationClosed()
	{
		auto * sender = QObject::sender();
		if (not sender) return;

		auto first = m_items.begin();
		auto last = m_items.end();
		auto it = std::find_if(first, last, [sender](auto & item) { return item.widget == sender; });
		if (it == last) return;

		auto item = std::move(*it);
		delete item.animation.data();

		m_items.erase(it);
		ScheduleUpdate();
	}

	QWidget * NotificationLayout::GetParent() const
	{
		return m_parent;
	}

	void NotificationLayout::SetParent(QWidget * widget)
	{
		m_parent = widget;
		ScheduleUpdate();
	}

	QRect NotificationLayout::GetGeometry() const
	{
		return m_geometry;
	}

	void NotificationLayout::SetGeometry(const QRect & geom)
	{
		m_geometry = geom;
		ScheduleUpdate();
	}

	Qt::Corner NotificationLayout::GetCorner() const
	{
		return m_corner;
	}

	void NotificationLayout::SetCorner(Qt::Corner corner)
	{
		m_corner = corner;
		ScheduleUpdate();
	}

	void NotificationLayout::ScheduleUpdate()
	{
		if (not m_relayoutScheduled)
		{
			QMetaObject::invokeMethod(this, "DoScheduledUpdate", Qt::QueuedConnection);
			m_relayoutScheduled = true;
		}
	}

	void NotificationLayout::DoScheduledUpdate()
	{
		m_relayoutScheduled = false;
		Relayout();
	}

	auto NotificationLayout::DescribeCorner(Qt::Corner corner) -> std::tuple<GetPointPtr, MovePointPtr, int>
	{
		switch (corner)
		{
			case Qt::TopLeftCorner:
				return std::make_tuple(&QRect::topLeft, &QRect::moveTopLeft, +1);

			case Qt::TopRightCorner:
				return std::make_tuple(&QRect::topRight, &QRect::moveTopRight, +1);

			case Qt::BottomLeftCorner:
				return std::make_tuple(&QRect::bottomLeft, &QRect::moveBottomLeft, -1);

			case Qt::BottomRightCorner:
			default: // treat anything other as BottomRightCorner
				return std::make_tuple(&QRect::bottomRight, &QRect::moveBottomRight, -1);
		}
	}

	QRect NotificationLayout::AlignRect(QRect rect, const QRect & parent, Qt::Corner corner)
	{
		auto [getter, setter, direction] = DescribeCorner(corner);
		Q_UNUSED(direction);

		(rect.*setter)((parent.*getter)());
		return rect;
	}

	QRect NotificationLayout::DefaultLayoutRect(const QRect & parent, Qt::Corner corner)
	{
		auto * desktop = qApp->desktop();
		QFont font = qApp->font();
		QFontMetrics fm(font);

		// calculate default rect based on parent geom and corner
		const auto minimumWidth = fm.averageCharWidth() * 40;
		const auto maximumWidth = fm.averageCharWidth() * 60;

		// minimum height - at least 4 rows of text(1 for title + 3 for main text) 
		// with some additional spacing
		const auto minimumHeight = fm.height() * 4 + 20;
		const auto maximumHeight = parent.height();

		auto width = parent.width() / 3;
		auto height = parent.height() / 3;

		width = qBound(minimumWidth, width, maximumWidth);
		height = qBound(minimumHeight, height, maximumHeight);

		auto [getter, setter, direction] = DescribeCorner(corner);
		Q_UNUSED(direction);

		QRect rect(0, 0, width, height);
		(rect.*setter)((parent.*getter)());

		return rect;
	}

	QRect NotificationLayout::ParentGeometry() const
	{
		if (m_parent)
			return m_parent->geometry();
		else
		{
			auto * desktop = qApp->desktop();
			return desktop->availableGeometry(desktop->primaryScreen());
		}
	}


	QRect NotificationLayout::CalculateLayoutRect() const
	{
		QRect parent = ParentGeometry();

		if (not m_geometry.isNull())
			return AlignRect(m_geometry, parent, m_corner);
		else
			return DefaultLayoutRect(parent, m_corner);
	}

	void NotificationLayout::Update()
	{
		Relayout();
	}

	void NotificationLayout::Relayout()
	{
		auto [getter, setter, direction] = DescribeCorner(m_corner);
		auto geometry = CalculateLayoutRect();

		QPoint cur = (geometry.*getter)();
		for (const auto & item : m_items)
		{
			auto * wgt = item.widget.data();
			wgt->adjustSize();

			QSize sz;
			auto hint = wgt->heightForWidth(geometry.width());
			if (hint < 0)
				sz = wgt->size();
			else
			{
				sz.rwidth() = geometry.width();
				sz.rheight() = hint;
			}

			cur.ry() += direction * ms_spacing;

			QRect geom(0, 0, sz.width(), sz.height());
			(geom.*setter)(cur);			

			wgt->setParent(m_parent);
			wgt->setGeometry(geom);
			wgt->show();

			cur.ry() += direction * sz.height();
		}
	}
}
