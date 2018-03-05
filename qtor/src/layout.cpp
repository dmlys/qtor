#include "layout.hqt"
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>

namespace QtTools::NotificationSystem
{
	unsigned NotificationLayout::NotificationsCount() const
	{
		return static_cast<unsigned>(m_items.size());
	}

	auto NotificationLayout::NotificationAt(unsigned index) -> QPointer<const Notification>
	{
		if (m_items.size() >= index)
			return {};

		return m_items[index].notification;
	}

	auto NotificationLayout::TakeNotification(unsigned index) -> QPointer<const Notification>
	{
		if (m_items.size() >= index)
			return {};

		auto item = std::move(m_items[index]);
		m_items.erase(m_items.begin() + index);

		item.widget->close();
		delete item.widget.data();

		ScheduleRelayout();
		return item.notification;
	}

	void NotificationLayout::AddNotification(QPointer<const Notification> notification, NotificationPopupWidget * widget)
	{
		Item item;
		item.notification = std::move(notification);
		item.widget = widget;

		m_items.push_back(std::move(item));
		ScheduleRelayout();
	}

	void NotificationLayout::AddNotification(QPointer<const Notification> notification)
	{
		Item item;
		item.notification = std::move(notification);
		item.widget = item.notification->CreatePopup();

		m_items.push_back(std::move(item));
		ScheduleRelayout();
	}

	QWidget * NotificationLayout::GetParent() const
	{
		return m_parent;
	}

	void NotificationLayout::SetParent(QWidget * widget)
	{
		m_parent = widget;
		ScheduleRelayout();
	}

	QRect NotificationLayout::GetGeometry() const
	{
		return m_geometry;
	}

	void NotificationLayout::SetGeometry(const QRect & geom)
	{
		m_geometry = geom;
		ScheduleRelayout();
	}

	Qt::Corner NotificationLayout::GetCorner() const
	{
		return m_corner;
	}

	void NotificationLayout::SetCorner(Qt::Corner corner)
	{
		m_corner = corner;
		ScheduleRelayout();
	}

	void NotificationLayout::ScheduleRelayout()
	{
		if (not m_relayoutScheduled)
		{
			QMetaObject::invokeMethod(this, "Relayout", Qt::QueuedConnection);
			m_relayoutScheduled = true;
		}
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

			cur.ry() += direction * m_spacing;

			QRect geom(0, 0, sz.width(), sz.height());
			(geom.*setter)(cur);			

			wgt->setParent(m_parent);
			wgt->setGeometry(geom);
			wgt->show();

			cur.ry() += direction * sz.height();
		}

		m_relayoutScheduled = false;
	}
}
