#include <QtCore/QEvent>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>

#include <qtor/NotificationSystem.hqt>
#include <qtor/NotificationSystemExt.hqt>
#include <qtor/NotificationLayout.hqt>
#include <qtor/NotificationLayoutExt.hqt>


namespace QtTools::NotificationSystem
{
	const NotificationLayout::CreatePopupFunction NotificationLayout::ms_defaultCreatePopup = 
		[](const Notification & n) { return new NotificationPopupWidget(n); };

	NotificationLayout::Item::~Item()
	{
		delete slideAnimation.data();
		delete moveOutAnimation.data();
		delete widget.data();
	}

	NotificationLayout::~NotificationLayout() = default;

	NotificationLayout::NotificationLayout(NotificationCenter & center, QObject * parent /* = nullptr */)
		: NotificationLayout(parent)
	{
		Init(center);
	}

	void NotificationLayout::Init(NotificationCenter & center)
	{
		SetNotificationCenter(&center);		
	}

	void NotificationLayout::SetNotificationCenter(NotificationCenter * center)
	{
		if (m_ncenter) m_ncenter->disconnect(this);
		
		m_ncenter = center;

		if (m_ncenter)
		{
			connect(m_ncenter, &NotificationCenter::NotificationAdded,
			        this, static_cast<void(NotificationLayout::*)(QPointer<const Notification>)>(&NotificationLayout::AddNotification));
		}
	}

	void NotificationLayout::SetCreatePopupFunction(CreatePopupFunction func)
	{
		if (not func)
		{
			m_createPopup = ms_defaultCreatePopup;
			return;
		}

		m_createPopup = std::move(func);
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
		item.widget->close();

		m_items.erase(m_items.begin() + index);

		ScheduleUpdate();
		return item.notification;
	}

	void NotificationLayout::AddNotification(QPointer<const Notification> notification)
	{
		Item item;
		item.notification = std::move(notification);

		m_items.push_back(std::move(item));		
		ScheduleUpdate();
	}
	
	void NotificationLayout::NotificationClosed()
	{
		auto * sender = QObject::sender();
		if (not sender) return;

		auto first = m_items.begin();
		auto last = m_items.end();
		auto it = std::find_if(first, last, [sender](auto & item) { return item.widget == sender; });
		if (it == last) return;

		it->widget = nullptr;
		m_items.erase(it);

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

	auto NotificationLayout::CreatePopup(const Notification * notification) const 
		-> AbstractNotificationPopupWidget *
	{
		auto * widget = m_createPopup(*notification);
		widget->setAttribute(Qt::WA_DeleteOnClose, true);
		widget->setParent(m_parent);
		widget->adjustSize();
		widget->hide();

		widget->installEventFilter(ext::unconst(this));

		connect(widget, &QObject::destroyed, this, &NotificationLayout::NotificationClosed);
		connect(widget, &AbstractNotificationPopupWidget::LinkActivated, this, &NotificationLayout::LinkActivated);
		connect(widget, &AbstractNotificationPopupWidget::LinkHovered,   this, &NotificationLayout::LinkHovered);
		
		return widget;
	}

	void NotificationLayout::SlideWidget(Item & item, const QRect & hgeom, const QRect & lgeom) const
	{
		auto * animation = item.slideAnimation.data();
		if (animation)
		{
			animation->pause();
			animation->setStartValue(hgeom);
			animation->setEndValue(lgeom);
			animation->resume();
		}
		else
		{
			auto * widget = item.widget.data();
			animation = new QPropertyAnimation(widget, "geometry", widget);
			animation->setEasingCurve(QEasingCurve::InCirc);
			animation->setStartValue(hgeom);
			animation->setEndValue(lgeom);

			animation->start(animation->DeleteWhenStopped);
			item.slideAnimation = animation;
		}
	}

	void NotificationLayout::Update()
	{
		if (m_relocation)
		{
			m_relocation = false;
			ClearAnimatedWidgets();
		}

		Relayout();
	}

	void NotificationLayout::Relayout()
	{
		auto [getter, setter, direction] = DescribeCorner(m_corner);
		auto geometry = CalculateLayoutRect();

		unsigned idx = 0;
		QPoint start = (geometry.*getter)();
		QPoint lcur = start;
		QPoint hcur = start;

		QRect hgeom, lgeom;
		QSize sz;

		for (auto & item : m_items)
		{
			auto * widget = item.widget.data();
			bool is_new = not widget;

			if (item.moveOutAnimation)
			{
				auto geom = widget->geometry();
				sz = geom.size();
				hcur = lcur = (geom.*getter)();

				goto loop_end;
			}
 
			lcur.ry() += direction * ms_spacing;
			hcur.ry() += direction * ms_spacing;

			if (is_new)
			{
				item.widget = widget = CreatePopup(item.notification);
				widget->show();
			}
			
			auto hint = widget->heightForWidth(geometry.width());
			if (hint < 0)
				sz = widget->size();
			else
			{
				sz.rwidth() = geometry.width();
				sz.rheight() = hint;
			}

			lgeom.setRect(0, 0, sz.width(), sz.height());
			(lgeom.*setter)(lcur);

			if (is_new)
			{
				hgeom.setRect(0, 0, sz.width(), sz.height());
				(hgeom.*setter)(hcur);

				widget->setGeometry(hgeom);
			}
			else
			{
				hgeom = widget->geometry();
				hcur = (hgeom.*getter)();
			}
			
			if (hcur != lcur)
				SlideWidget(item, hgeom, lgeom);

		loop_end:
			lcur.ry() += direction * sz.height();
			hcur.ry() += direction * sz.height();

			if (++idx >= m_widgetsLimit)
				break;

			if (std::abs(lcur.y() - start.y()) >= geometry.height())
				break;
		}
	}

	void NotificationLayout::ClearAnimatedWidgets()
	{
		auto first = m_items.begin();
		auto last = m_items.end();
		first = std::remove_if(first, last, [](auto & item) { return item.moveOutAnimation; });
		m_items.erase(first, last);		
	}

	bool NotificationLayout::eventFilter(QObject * watched, QEvent * event)
	{
		if (event->type() != QEvent::MouseButtonPress)
			return false;

		auto * ev = static_cast<QMouseEvent *>(event);
		if (ev->button() != Qt::RightButton) return false;
		
		auto first = m_items.begin();
		auto last = m_items.end();
		auto it = std::find_if(first, last, [watched](auto & item) { return item.widget == watched; });
		if (it == last) return false;

		auto * widget = it->widget.data();

		if (not widget->contentsRect().contains(ev->pos()))
			return false;

		if (not it->slideAnimation)
			it->moveOutAnimation = widget->MoveOutAndClose();
		return true;
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
}
