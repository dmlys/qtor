#include <qtor/NotificationPopupWidget.hqt>

#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QStylePainter>
#include <QtWidgets/QGraphicsDropShadowEffect>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>

#include <QtCore/QPropertyAnimation>

// src/widgets/effects/qpixmapfilter.cpp
Q_DECL_IMPORT void qt_blurImage(
	QPainter * p, QImage & blurImage,
	qreal radius, bool quality, bool alphaOnly, int transposed = 0
);

namespace QtTools
{
	QMarginsF NotificationPopupWidget::ShadowMargins() const noexcept
	{
		auto blur_radius = m_effect->blurRadius();
		auto blur_offset = m_effect->offset();

		return {
			std::max(blur_radius - blur_offset.x(), 0.0),
			std::max(blur_radius - blur_offset.y(), 0.0),
			blur_radius + blur_offset.x(),
			blur_radius + blur_offset.y(),
		};
	}

	QMargins NotificationPopupWidget::FrameMargins() const noexcept
	{
		auto frame_width = m_framePen.width();
		return {frame_width, frame_width, frame_width, frame_width};
	}

	QRectF NotificationPopupWidget::DropShadowEffect::boundingRectFor(const QRectF & rect) const
	{
		// this is original calculation
		//auto radius = blurRadius();
		//return rect | rect.translated(offset()).adjusted(-radius, -radius, radius, radius);
		
		// but we return rect as is(see NotificationPopupWidget description)
		return rect;
	}

	void NotificationPopupWidget::DropShadowEffect::draw(QPainter * painter)
	{
		if (blurRadius() <= 0 && offset().isNull())
		{
			drawSource(painter);
			return;
		}

		// Draw pixmap in device coordinates to avoid pixmap scaling.
		QPoint offset;
		const QPixmap pixmap = sourcePixmap(Qt::DeviceCoordinates, &offset, PadToEffectiveBoundingRect);
		if (pixmap.isNull())
			return;

		QTransform restoreTransform = painter->worldTransform();
		painter->setWorldTransform(QTransform());

		// NOTE: pixmap contains our whole widget rect.
		//   rect = ShadowMargins + FrameMargins + contentsRect
		auto * owner = this->owner();
		auto margins = owner->contentsMargins();
		auto frameMargins = owner->FrameMargins();

		// draw pixmap into tmp for further processing(blurring etc)
		QImage tmp(pixmap.size(), QImage::Format_ARGB32_Premultiplied);
		tmp.setDevicePixelRatio(pixmap.devicePixelRatioF());
		tmp.fill(0);
		QPainter tmpPainter(&tmp);
		tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
		tmpPainter.drawPixmap(this->offset(), pixmap);
		tmpPainter.end();

		// blur the alpha channel from tmp into blurPainter(blurred)
		QImage blurred(tmp.size(), QImage::Format_ARGB32_Premultiplied);
		blurred.setDevicePixelRatio(pixmap.devicePixelRatioF());
		blurred.fill(0);
		QPainter blurPainter(&blurred);
		qt_blurImage(&blurPainter, tmp, blurRadius(), false, true);
		blurPainter.end();

		tmp = blurred;

		// blacken blurred shadow image
		tmpPainter.begin(&tmp);
		tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		tmpPainter.fillRect(tmp.rect(), color());
		tmpPainter.end();

		// draw the blurred drop shadow into resulting painter
		painter->drawImage(offset, tmp);

		// Draw the actual pixmap, but only contents + frame
		auto old = painter->compositionMode();
		painter->setCompositionMode(QPainter::CompositionMode_Source);
		painter->drawPixmap(offset, pixmap, pixmap.rect() - margins + frameMargins);
		painter->setCompositionMode(old);

		painter->setWorldTransform(restoreTransform);
	}

	QSize NotificationPopupWidget::sizeHint() const
	{
		auto margins = contentsMargins();
		
		return QWidget::sizeHint() + QSize {
			margins.left() + margins.right(),
			margins.top() + margins.bottom(),
		};
	}

	void NotificationPopupWidget::paintEvent(QPaintEvent * ev)
	{
		QPainter painter(this);

		painter.setPen(m_framePen);
		painter.setBrush(m_backgroundBrush);
		
		auto frameWidth = m_framePen.width();
		painter.drawRect(contentsRect().adjusted(-frameWidth, -frameWidth, 0, 0));
	}

	void NotificationPopupWidget::mousePressEvent(QMouseEvent * ev)
	{
		if (ev->button() == Qt::RightButton and contentsRect().contains(ev->pos()))
			MoveOutAndClose();

		return base_type::mousePressEvent(ev);
	}

	QAbstractAnimation * NotificationPopupWidget::CreateMoveOutAnimation()
	{
		auto * animation = new QPropertyAnimation(this, "geometry", this);

		QRectF parentGeom;
		auto * parent = this->parentWidget();
		if (parent)
			parentGeom = parent->geometry();
		else
			parentGeom = qApp->desktop()->availableGeometry(this);

		QRectF start = this->geometry();
		QRectF finish = parentGeom.right() - start.right() < start.left() - parentGeom.left()
			? QRectF(parentGeom.right(), start.top(), 0, start.height())
			: QRectF(parentGeom.left(), start.top(), 0, start.height());

		animation->setStartValue(start);
		animation->setEndValue(finish);
		animation->setEasingCurve(QEasingCurve::InCirc);

		return animation;
	}

	void NotificationPopupWidget::MoveOutAndClose()
	{
		auto * animation = CreateMoveOutAnimation();
		connect(animation, &QPropertyAnimation::finished, this, &NotificationPopupWidget::close);
		connect(animation, &QPropertyAnimation::finished, this, &NotificationPopupWidget::MovedOut);

		animation->setParent(this);
		animation->start(animation->DeleteWhenStopped);
	}

	NotificationPopupWidget::NotificationPopupWidget(QWidget * parent /* = nullptr */, Qt::WindowFlags flags /* = {} */)
		: QWidget(parent, flags)
	{
		setWindowFlag(Qt::FramelessWindowHint);
		setAttribute(Qt::WA_OpaquePaintEvent);
		setAttribute(Qt::WA_DeleteOnClose);

		if (isWindow())
			setAttribute(Qt::WA_TranslucentBackground);

		auto * shadowEffect = new DropShadowEffect(this);
		shadowEffect->setBlurRadius(4.0);
		shadowEffect->setColor(palette().color(QPalette::Shadow));
		shadowEffect->setOffset(4.0);
		this->setGraphicsEffect(shadowEffect);
		
		m_effect = shadowEffect;
		
		auto margins = ShadowMargins() + FrameMargins();
		setContentsMargins(margins.toMargins());
	}
}
