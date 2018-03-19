#include <tuple>
#include <ext/utility.hpp>

#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaMethod>
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
		[](const Notification & n, const NotificationLayout & that)
		{
			auto * widget = that.CreatePopup(n);
			that.CustomizePopup(n, widget);
			that.ConfigureExpiration(n, widget);
			return widget;
		};

	NotificationLayout::Item::~Item()
	{
		delete slideAnimation.data();
		delete moveOutAnimation.data();
		delete widget.data();
	}

	NotificationLayout::~NotificationLayout() = default;


	void NotificationLayout::InitColors()
	{
		m_errorColor = QColor("red");
		m_errorColor.setAlpha(200);

		m_warnColor = QColor("yellow");
		m_warnColor.setAlpha(200);

		m_infoColor = QColor("silver");
		m_infoColor.setAlpha(200);
	}

	NotificationLayout::NotificationLayout(QObject * parent /* = nullptr */)
		: QObject(parent)
	{
		InitColors();
	}

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

	auto NotificationLayout::CreatePopup(const Notification & notification) const
		-> AbstractNotificationPopupWidget *
	{
		// try create using Q_INVOKABLE createPopup method of notification,
		// if failed fallback to NotificationPopupWidget.
		auto * meta = notification.metaObject();
		int index = meta->indexOfMethod("createPopup()");
		if (index >= 0)
		{
			AbstractNotificationPopupWidget * result = nullptr;
			QMetaMethod method = meta->method(index);
			if (method.invoke(ext::unconst(&notification), Qt::DirectConnection, Q_RETURN_ARG(QtTools::NotificationSystem::AbstractNotificationPopupWidget *, result)))
				return result;
		}
				
		return new NotificationPopupWidget(notification);
	}

	void NotificationLayout::CustomizePopup(const Notification & n, AbstractNotificationPopupWidget * popup) const
	{	
		if (auto color = qvariant_cast<QColor>(n.property("backgroundColor")); color.isValid())
			popup->SetBackgroundBrush(color);
		else if (auto brush = qvariant_cast<QBrush>(n.property("backgroundBrush")); brush != Qt::NoBrush)
			popup->SetBackgroundBrush(brush);
		else
		{
			static constexpr auto getter = [](auto val) { return val; };
			unsigned lvl = n.Level() - NotificationLevel::Error;

			auto colors = GetColors();
			auto color = ext::visit(colors, lvl, getter);
			popup->SetBackgroundBrush(color);
		}
	}

	void NotificationLayout::ConfigureExpiration(const Notification & n, AbstractNotificationPopupWidget * popup) const
	{
		int msecs = 0;

		auto prop = n.property("expirationTimeout");
		if (prop.canConvert<int>())
			msecs = qvariant_cast<int>(prop);
		else
		{
			static constexpr auto getter = [](auto val) { return val; };
			unsigned lvl = n.Level() - NotificationLevel::Error;

			auto timeouts = GetExpirationTimeouts();
			std::chrono::milliseconds timeout = ext::visit(timeouts, lvl, getter);
			msecs = timeout.count();
		}

		if (msecs != 0)
		{
			auto moveOut = [popup, that = ext::unconst(this)]() mutable
			{
				auto first = that->m_items.begin();
				auto last = that->m_items.end();
				auto it = std::find_if(first, last, [popup](auto & item) { return item.widget == popup; });

				if (it != last and not it->slideAnimation and not it->moveOutAnimation)
					it->moveOutAnimation = it->widget->MoveOutAndClose();
			};

			QTimer::singleShot(msecs, popup, moveOut);
		}
			
	}

	auto NotificationLayout::MakePopup(const Notification * notification) const 
		-> AbstractNotificationPopupWidget *
	{
		auto * widget = m_createPopup(*notification, *this);
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
			if (idx++ >= m_widgetsLimit)
				break;

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
				item.widget = widget = MakePopup(item.notification);
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

		if (not it->slideAnimation and not it->moveOutAnimation)
			it->moveOutAnimation = widget->MoveOutAndClose();

		return true;
	}

	void NotificationLayout::SetParent(QWidget * widget)
	{
		m_parent = widget;
		ScheduleUpdate();
	}

	void NotificationLayout::SetGeometry(const QRect & geom)
	{
		m_geometry = geom;
		ScheduleUpdate();
	}

	void NotificationLayout::SetCorner(Qt::Corner corner)
	{
		m_corner = corner;
		ScheduleUpdate();
	}

	void NotificationLayout::SetWidgetsLimit(unsigned limit)
	{
		m_widgetsLimit = limit;
	}

	auto NotificationLayout::GetColors() const -> std::tuple<QColor, QColor, QColor>
	{
		return std::make_tuple(m_errorColor, m_warnColor, m_infoColor);
	}

	void NotificationLayout::SetColors(QColor error, QColor warning, QColor info)
	{
		m_errorColor = error;
		m_warnColor = warning;
		m_infoColor = info;
	}

	auto NotificationLayout::GetExpirationTimeouts() const
		-> std::tuple<std::chrono::milliseconds, std::chrono::milliseconds, std::chrono::milliseconds>
	{
		return std::make_tuple(m_errorTimeout, m_warnTimeout, m_infoTimeout);
	}

	void NotificationLayout::SetExpirationTimeouts(std::chrono::milliseconds error, std::chrono::milliseconds warning, std::chrono::milliseconds info)
	{
		m_errorTimeout = error;
		m_warnTimeout = warning;
		m_infoTimeout = info;
	}
}
