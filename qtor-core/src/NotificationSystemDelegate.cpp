#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QTextLayout>
#include <QtGui/QTextDocument>
#include <QtGui/QAbstractTextDocumentLayout>
#include <QtWidgets/QAbstractItemView>

#include <QtTools/Delegates/Utils.hpp>
#include <QtTools/Delegates/StyledParts.hpp>
#include <QtTools/Delegates/DrawFormattedText.hpp>

#include <qtor/NotificationSystem.hqt>
#include <qtor/NotificationSystemExt.hqt>


namespace QtTools
{
	const unsigned NotificationSystem::SimpleNotification::ms_InnerMargins = 1;
	const QMargins NotificationSystem::SimpleNotification::ms_OutterMargins = {0, 0, 0, 0};
	
	static const char * ms_OldHrefProperty = "QtTools::NotificationSystem::SimpleNotification::OldHref";
	static const QVariant ms_NullVariant;

	QString NotificationSystem::SimpleNotification::Text() const
	{
		return m_text;
	}

	QDateTime NotificationSystem::SimpleNotification::Timestamp() const
	{
		return m_timestamp;
	}

	NotificationPopupWidget * NotificationSystem::SimpleNotification::CreatePopup() const
	{
		return nullptr;
	}

	QMargins NotificationSystem::SimpleNotification::TextMargins(const QStyleOptionViewItem & option)
	{
		return ms_OutterMargins + QtTools::Delegates::TextMargins(option);
	}

	void NotificationSystem::SimpleNotification::PrepareTextDocument(QTextDocument & textDoc, const LaidoutItem & item)
	{
		QPaintDevice * device = const_cast<QWidget *>(item.option->widget);

		textDoc.setDefaultTextOption(QtTools::Delegates::TextLayout::PrepareTextOption(*item.option));
		textDoc.setDocumentMargin(0); // be default it's == 4
		textDoc.setDefaultFont(item.textFont);
		textDoc.documentLayout()->setPaintDevice(device);
		textDoc.setHtml(item.text);
	}

	void NotificationSystem::SimpleNotification::LayoutItem(const QStyleOptionViewItem & option, LaidoutItem & item) const 
	{
		using namespace QtTools::Delegates::TextLayout;

		item.option = &option;
		if (item.notification == this)
		{
			auto newTopLeft = option.rect.topLeft();
			auto diff = option.rect.topLeft() - item.hintTopLeft;
			item.hintTopLeft = option.rect.topLeft();

			item.titleRect.translate(diff);
			item.timestampRect.translate(diff);
			item.textRect.translate(diff);
			item.pixmapRect.translate(diff);

			return;
		}
		
		const auto margins = TextMargins(option);
		const auto rect = option.rect - margins;
		const auto topLeft = rect.topLeft();

		item.notification = this;
		item.hintTopLeft = option.rect.topLeft();
		item.timestamp = m_timestamp.toString(Qt::DateFormat::DefaultLocaleShortDate);
		item.title = m_title;
		item.text = m_text;
		item.pixmap = m_pixmap;

		item.textFont = item.titleFont = item.timestampFont = item.baseFont = option.font;
		item.titleFont.setPointSize(item.titleFont.pointSize() * 11 / 10);
		item.titleFont.setBold(true);

		//item.pixmapRect = QRect({0, 0}, item.pixmap.size()) + ms_InnerMargins;
		//item.pixmapRect.moveTo(topLeft);
		//auto pixmapSz = m_pixmap.size();

		QPaintDevice * device = const_cast<QWidget *>(option.widget);
		QFontMetrics titleFm {item.titleFont, device};
		QFontMetrics textFm {item.textFont, device};
		QFontMetrics timestampFm {item.timestampFont, device};
		QSize timestampSz(timestampFm.width(item.timestamp), titleFm.height());

		// title size is drawn in left top corner, no more than 2 lines, with 40 average chars as width
		QSize titleSz {40 * titleFm.averageCharWidth(), 2 * titleFm.height()};
		const auto titleSpacer = 2 * titleFm.averageCharWidth();
		
		QTextLayout layout(m_title, item.titleFont, device);
		layout.setTextOption(PrepareTextOption(option));

		int elideIndex = DoLayout(layout, QRectF({0.0, 0.0}, titleSz));
		titleSz = NaturalBoundingRect(layout, elideIndex).size().toSize();
		
		item.titleRect = {topLeft, titleSz};
		item.timestampRect = QRect {
			{ item.titleRect.right() + titleSpacer, topLeft.y() },
			timestampSz
		};

		item.textdocptr = std::make_shared<QTextDocument>();
		QTextDocument & textDoc = *item.textdocptr;
		PrepareTextDocument(textDoc, item);
		textDoc.setTextWidth(titleSz.width() + timestampSz.width() + titleSpacer);

		QSize textSz {std::lround((textDoc.idealWidth())), std::lround(textDoc.size().height())};
		int textTop = ms_InnerMargins + std::max(item.titleRect.bottom(), item.timestampRect.bottom());
		item.textRect = QRect {{topLeft.x(), textTop}, textSz};

		item.totalRect = item.timestampRect | item.titleRect | item.textRect;
		item.totalRect += margins;
		
		return;
	}

	void NotificationSystem::SimpleNotification::Draw(QPainter * painter, const LaidoutItem & item) const
	{
		using namespace QtTools::Delegates;

		const QStyleOptionViewItem & option = *item.option;
		const bool selected = option.state & QStyle::State_Selected;		
		const auto margins = TextMargins(option);
		const auto rect = FocusFrameSubrect(option) - margins;

		auto cg = QtTools::Delegates::ColorGroup(option);
		auto cr = selected ? QPalette::HighlightedText : QPalette::Text;

		auto & titleRect = item.titleRect;
		auto timestampRect = item.timestampRect;
		timestampRect.moveRight(rect.right());

		painter->setPen(option.palette.color(cg, cr));
		
		auto titleopt = option;
		titleopt.font = item.titleFont;
		DrawFormattedText(painter, item.title, item.titleRect, titleopt);

		painter->setFont(item.timestampFont);
		painter->drawText(timestampRect, item.timestamp);


		painter->save();
		painter->translate(item.textRect.topLeft());
		
		assert(item.textdocptr);
		QTextDocument & textDoc = *item.textdocptr;
		textDoc.drawContents(painter);

		painter->restore();
	}

	void NotificationSystem::SimpleNotification::DrawBackground(QPainter * painter, const LaidoutItem & item) const
	{
		using namespace QtTools::Delegates;
		const QStyleOptionViewItem & option = *item.option;
		const bool selected = option.state & QStyle::State_Selected;

		if (selected)
		{
			auto cg = ColorGroup(option);
			painter->fillRect(item.option->rect, option.palette.brush(cg, QPalette::Highlight));
		}
	}

	void NotificationSystem::SimpleNotification::paint(QPainter * painter, const QStyleOptionViewItem & option, std::any & cookie) const
	{
		auto * cache = std::any_cast<LaidoutItem>(&cookie);
		if (not cache) cache = &cookie.emplace<LaidoutItem>();

		LayoutItem(option, *cache);

		DrawBackground(painter, *cache);
		Draw(painter, *cache);

		using namespace QtTools::Delegates;
		if (HasFocusFrame(option))
			DrawFocusFrame(painter, cache->option->rect, option);
	}

	QSize NotificationSystem::SimpleNotification::sizeHint(const QStyleOptionViewItem & option, std::any & cookie) const
	{
		auto * cache = std::any_cast<LaidoutItem>(&cookie);
		if (not cache) cache = &cookie.emplace<LaidoutItem>();
		
		LayoutItem(option, *cache);
		return cache->totalRect.size();
	}

	bool NotificationSystem::SimpleNotification::editorEvent(QEvent * event, const QStyleOptionViewItem & option, std::any & cookie) const
	{
		auto evType = event->type();
		bool mouseButtonEvent =
			   evType == QEvent::MouseButtonPress
			or evType == QEvent::MouseMove;

		if (not mouseButtonEvent) return false;
		
		auto * me = static_cast<QMouseEvent *>(event);

		auto * cache = std::any_cast<LaidoutItem>(&cookie);
		if (not cache) cache = &cookie.emplace<LaidoutItem>();

		auto & item = *cache;
		LayoutItem(option, item);

		assert(item.textdocptr);
		QTextDocument & textDoc = *item.textdocptr;
		auto * docLayout = textDoc.documentLayout();
		auto * listView = qobject_cast<const QAbstractItemView *>(option.widget);
		auto * viewport = listView->viewport();
		
		auto docClickPos = me->pos() - item.textRect.topLeft();
		QString href = docLayout->anchorAt(docClickPos);

		if (evType == QEvent::MouseMove)
		{
			auto oldHref = qvariant_cast<QString>(viewport->property(ms_OldHrefProperty));
			if (href.isEmpty())
			{
				if (not oldHref.isNull())
				{
					viewport->setProperty(ms_OldHrefProperty, ms_NullVariant);
					LinkUnhovered(std::move(oldHref), option);
				}
			}
			else
			{
				if (oldHref.isEmpty())
				{
					viewport->setProperty(ms_OldHrefProperty, href);
					LinkHovered(std::move(href), option);
				}
			}

			return true;
		}
		else if (evType == QEvent::MouseButtonPress and not href.isEmpty())
		{
			LinkActivated(std::move(href), option);
			return true;
		}

		return false;
	}

	void NotificationSystem::SimpleNotification::LinkActivated(QString href, const QStyleOptionViewItem & option) const
	{
		qDebug() << "LinkActivated: " << href;
	}

	void NotificationSystem::SimpleNotification::LinkHovered(QString href, const QStyleOptionViewItem & option) const
	{
		auto * listView = qobject_cast<const QAbstractItemView *>(option.widget);
		auto * viewport = listView->viewport();

		viewport->setCursor(Qt::PointingHandCursor);
		qDebug() << "LinkHovered: " << href;
	}

	void NotificationSystem::SimpleNotification::LinkUnhovered(QString href, const QStyleOptionViewItem & option) const
	{
		auto * listView = qobject_cast<const QAbstractItemView *>(option.widget);
		auto * viewport = listView->viewport();

		viewport->unsetCursor();
		qDebug() << "LinkUnhovered: " << href;
	}

	/************************************************************************/
	/*                   SimpleNotificationDelegate                         */
	/************************************************************************/
	void NotificationSystem::SimpleNotificationDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
	{
		auto * model = dynamic_cast<const Model *>(index.model());
		const auto * notification = model->GetItem(index.row());
		return notification->paint(painter, option, m_cachedItem);
	}

	QSize NotificationSystem::SimpleNotificationDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
	{
		auto * model = dynamic_cast<const Model *>(index.model());
		const auto * notification = model->GetItem(index.row());
		return notification->sizeHint(option, m_cachedItem);
	}

	bool NotificationSystem::SimpleNotificationDelegate::editorEvent(QEvent * event, QAbstractItemModel *, const QStyleOptionViewItem & option, const QModelIndex & index)
	{
		auto * model = dynamic_cast<const Model *>(index.model());
		const auto * notification = model->GetItem(index.row());
		return notification->editorEvent(event, option, m_cachedItem);
	}
}
