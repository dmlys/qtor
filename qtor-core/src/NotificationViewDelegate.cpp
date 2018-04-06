#pragma once
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QTextLayout>
#include <QtGui/QTextDocument>
#include <QtGui/QAbstractTextDocumentLayout>
#include <QtWidgets/QAbstractItemView>

#include <QtTools/Delegates/Utils.hpp>
#include <QtTools/Delegates/StyledParts.hpp>
#include <QtTools/Delegates/DrawFormattedText.hpp>
#include <QtTools/Delegates/SearchDelegate.hpp>

#include <ext/utility.hpp>
#include <qtor/NotificationViewDelegate.hqt>


namespace QtTools::NotificationSystem
{
	const unsigned NotificationViewDelegate::ms_Spacing = 1;
	const QMargins NotificationViewDelegate::ms_ContentMargins = {1, 3, 1, 3};
	const QTextCharFormat NotificationViewDelegate::ms_searchFormat = [] 
	{
		QTextCharFormat format;
		format.setForeground(Qt::GlobalColor::red);
		format.setBackground(Qt::GlobalColor::green);
		return format;
	}();
	
	static const char * ms_OldHrefProperty = "QtTools::NotificationSystem::NotificationViewDelegate::OldHref";
	static const QVariant ms_NullVariant;

	QMargins NotificationViewDelegate::TextMargins(const QStyleOptionViewItem & option)
	{
		return ms_ContentMargins + QtTools::Delegates::TextMargins(option);
	}

	static void SetText(QTextDocument & textDoc, Qt::TextFormat fmt, QString text)
	{
		switch (fmt)
		{
			case Qt::PlainText: return textDoc.setPlainText(text);
			case Qt::RichText:  return textDoc.setHtml(text);

			case Qt::AutoText:
			default:

				if (Qt::mightBeRichText(text))
					return textDoc.setHtml(text);
				else
					return textDoc.setPlainText(text);

		}
	}

	QPixmap NotificationViewDelegate::GetPixmap(const Notification & notification, const QStyleOptionViewItem & option) const
	{
		QIcon ico;
		switch (notification.Level())
		{
			case QtTools::NotificationSystem::Error: ico = m_errorIcon; break;
			case QtTools::NotificationSystem::Warn:  ico = m_warnIcon;  break;
			case QtTools::NotificationSystem::Info:  ico = m_infoIcon;  break;
		}	

		auto * style = option.widget->style();
		int listViewIcoSz = style->pixelMetric(QStyle::PM_ListViewIconSize);
		QSize wantedsz = {listViewIcoSz, listViewIcoSz};
		return ico.pixmap(wantedsz);
	}

	void NotificationViewDelegate::LayoutTitle(const QStyleOptionViewItem & option, LaidoutItem & item) const
	{
		using namespace QtTools::Delegates;
		using namespace QtTools::Delegates::TextLayout;

		const auto margins = TextMargins(option);
		const auto rect = option.rect - margins;
		const auto topLeft = rect.topLeft();

		QPaintDevice * device = const_cast<QWidget *>(option.widget);
		const QFontMetrics titleFm {item.titleFont, device};
		const QFontMetrics timestampFm {item.timestampFont, device};

		// title size is drawn in left top corner, no more than 2 lines, with 40 average chars as width
		const QSize timestampSz = {timestampFm.width(item.timestamp), titleFm.height()};
		const auto titleSpacer = 2 * titleFm.averageCharWidth();

		const auto formats = FormatSearchText(item.title, item.searchStr, ms_searchFormat);
		const auto textopt = PrepareTextOption(option);

		// title size is drawn in left top corner, 
		// no more than 2 lines, with 40 average chars as width
		const QSize pixsz = item.pixmap.size();
		const qreal height = 2 * titleFm.height();
		const qreal width = std::max(40 * titleFm.averageCharWidth(), rect.width() - timestampSz.width() - titleSpacer);

		item.titleLayoutPtr = std::make_shared<QTextLayout>(item.title, item.titleFont, device);
		auto * layout = item.titleLayoutPtr.get();
		layout->setCacheEnabled(true);
		layout->setTextOption(textopt);
		layout->setFormats(formats);

		qreal cury = 0;
		int elideIndex = 0;
		layout->beginLayout();
		for (;;)
		{
			auto line = layout->createLine();
			if (!line.isValid()) break;

			qreal posx = cury < pixsz.height() ? pixsz.width() + ms_Spacing : 0;
			line.setPosition({posx, cury});
			line.setLineWidth(width - posx);
			cury += line.height();

			// last line didn't fit given total height 
			if (cury > height)
			{
				elideIndex = qMax(0, elideIndex - 1);
				break;
			}

			// very long word, does not fit given width
			if (line.naturalTextWidth() > width)
				break;

			++elideIndex;
		};

		layout->endLayout();
		bool needsElide = elideIndex != layout->lineCount();

		if (needsElide)
		{
			const auto & title = item.title;
			auto line = layout->lineAt(elideIndex);
			int elidePoint = line.textStart();
			item.title = title.mid(0, elidePoint) + ElideText(titleFm, title.mid(elidePoint), option.textElideMode, line.width());

			auto elidedFormats = ElideFormats(formats, elidePoint);
			ColorifyElidePoint(title, elidedFormats);

			item.titleLayoutPtr = nullptr;
			item.titleLayoutPtr = std::make_shared<QTextLayout>(title, item.title, device);
			layout = item.titleLayoutPtr.get();
			layout->setTextOption(textopt);
			layout->setFormats(elidedFormats);
			layout->setCacheEnabled(true);

			qreal cury = 0;
			layout->beginLayout();
			for (;;)
			{
				auto line = layout->createLine();
				if (!line.isValid()) break;

				qreal posx = cury < pixsz.height() ? pixsz.width() + ms_Spacing : 0;
				line.setPosition({posx, cury});
				line.setLineWidth(width - posx);
				cury += line.height();

				// last line didn't fit given total height 
				if (cury > height) break;
				// very long word, does not fit given width
				if (line.naturalTextWidth() > width) break;
			};

			layout->endLayout();
		}

		const auto titleSz = NaturalBoundingRect(*layout, elideIndex).size().toSize();

		item.pixmapRect = {topLeft, pixsz};
		item.titleRect = {topLeft, titleSz};
		item.timestampRect = QRect {
			{item.titleRect.right() + titleSpacer, topLeft.y()},
			timestampSz
		};
	}

	void NotificationViewDelegate::LayoutText(const QStyleOptionViewItem & option, LaidoutItem & item) const
	{
		const auto margins = TextMargins(option);
		const auto rect = option.rect - margins;
		const auto topLeft = rect.topLeft();

		QPaintDevice * device = const_cast<QWidget *>(option.widget);
		const QFontMetrics titleFm {item.titleFont, device};

		const auto titleSz = item.titleRect.size();
		const auto timestampSz = item.timestampRect.size();
		const auto titleSpacer = 2 * titleFm.averageCharWidth();

		item.textdocptr = std::make_shared<QTextDocument>();
		auto * highlighter = new SearchHighlighter(item.textdocptr.get());
		highlighter->SetSearchText(item.searchStr);
		highlighter->SetFormat(ms_searchFormat);

		QTextDocument & textDoc = *item.textdocptr;
		textDoc.setDefaultTextOption(QtTools::Delegates::TextLayout::PrepareTextOption(*item.option));
		textDoc.setDocumentMargin(0); // by default it's == 4
		textDoc.setDefaultFont(item.textFont);
		textDoc.documentLayout()->setPaintDevice(device);
		SetText(textDoc, item.textFormat, item.text);

		const auto width = std::max(rect.width(), titleSz.width() + timestampSz.width() + titleSpacer);
		textDoc.setTextWidth(width);

		QSize textSz {std::lround(textDoc.idealWidth()), std::lround(textDoc.size().height())};
		int textTop = ms_Spacing + std::max({item.pixmapRect.bottom(), item.titleRect.bottom(), item.timestampRect.bottom()});
		item.textRect = QRect {{topLeft.x(), textTop}, textSz};

	}

	void NotificationViewDelegate::LayoutItem(const QStyleOptionViewItem & option, LaidoutItem & item) const 
	{
		using namespace QtTools::Delegates::TextLayout;

		if (item.index == option.index)
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

		item.option = &option;
		auto * model = dynamic_cast<const AbstractNotificationModel *>(option.index.model());
		if (model) item.searchStr = model->GetFilter();

		const auto locale = option.widget->locale();
		const auto & notification = model->GetItem(option.index.row());
		const auto margins = TextMargins(option);
		const auto rect = option.rect - margins;
		const auto topLeft = rect.topLeft();

		item.hintTopLeft = option.rect.topLeft();
		item.timestamp = locale.toString(notification.Timestamp(), QLocale::ShortFormat);
		item.title = notification.Title();
		item.text = notification.Text();
		item.textFormat = notification.TextFmt();
		item.pixmap = GetPixmap(notification, option);

		item.textFont = item.titleFont = item.timestampFont = item.baseFont = option.font;
		item.titleFont.setPointSize(item.titleFont.pointSize() * 11 / 10);
		item.titleFont.setBold(true);

		LayoutTitle(option, item);
		LayoutText(option, item);

		item.totalRect = item.pixmapRect | item.timestampRect | item.titleRect | item.textRect;
		item.totalRect += margins;
		
		return;
	}

	void NotificationViewDelegate::Draw(QPainter * painter, const LaidoutItem & item) const
	{
		using namespace QtTools::Delegates;

		painter->drawPixmap(item.pixmapRect, item.pixmap);

		const QStyleOptionViewItem & option = *item.option;
		const bool selected = option.state & QStyle::State_Selected;
		const auto margins = TextMargins(option);
		const auto rect = FocusFrameSubrect(option) - margins;

		auto cg = ColorGroup(option);
		auto cr = selected ? QPalette::HighlightedText : QPalette::Text;

		auto & titleRect = item.titleRect;
		auto timestampRect = item.timestampRect;
		timestampRect.moveRight(rect.right());

		painter->setPen(option.palette.color(cg, cr));
		
		auto titleopt = option;
		titleopt.font = item.titleFont;
		TextLayout::DrawLayout(painter, item.titleRect.topLeft(), *item.titleLayoutPtr);

		painter->setFont(item.timestampFont);
		painter->drawText(timestampRect, item.timestamp);


		painter->save();
		painter->translate(item.textRect.topLeft());
		
		assert(item.textdocptr);
		QTextDocument & textDoc = *item.textdocptr;
		textDoc.setTextWidth(rect.width());
		textDoc.drawContents(painter);

		painter->restore();
	}

	void NotificationViewDelegate::DrawBackground(QPainter * painter, const LaidoutItem & item) const
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

	void NotificationViewDelegate::init(const QStyleOptionViewItem & option, const QModelIndex & index) const
	{
		auto & opt = ext::unconst(option);
		opt.index = index;
	}

	void NotificationViewDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
	{
		init(option, index);
		LayoutItem(option, m_cachedItem);

		DrawBackground(painter, m_cachedItem);
		Draw(painter, m_cachedItem);

		using namespace QtTools::Delegates;
		if (HasFocusFrame(option))
			DrawFocusFrame(painter, m_cachedItem.option->rect, option);
	}

	QSize NotificationViewDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
	{
		// sizeHint should always be recalculated
		// force it by setting invalid index
		m_cachedItem.index = {};

		init(option, index);
		LayoutItem(option, m_cachedItem);

		return m_cachedItem.totalRect.size();
	}

	bool NotificationViewDelegate::editorEvent(QEvent * event, QAbstractItemModel *, const QStyleOptionViewItem & option, const QModelIndex & index)
	{
		auto evType = event->type();
		bool mouseButtonEvent =
			   evType == QEvent::MouseButtonPress
			or evType == QEvent::MouseMove;

		if (not mouseButtonEvent) return false;
		
		auto * me = static_cast<QMouseEvent *>(event);

		init(option, index);		
		LayoutItem(option, m_cachedItem);

		assert(m_cachedItem.textdocptr);
		QTextDocument & textDoc = *m_cachedItem.textdocptr;
		auto * docLayout = textDoc.documentLayout();
		auto * listView = qobject_cast<const QAbstractItemView *>(option.widget);
		auto * viewport = listView->viewport();
		
		auto docClickPos = me->pos() - m_cachedItem.textRect.topLeft();
		QString href = docLayout->anchorAt(docClickPos);

		if (evType == QEvent::MouseMove)
		{
			auto oldHref = qvariant_cast<QString>(viewport->property(ms_OldHrefProperty));
			if (href.isEmpty() xor oldHref.isEmpty())
			{
				viewport->setProperty(ms_OldHrefProperty, href);
				LinkHovered(std::move(href), option);
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

	bool NotificationViewDelegate::helpEvent(QHelpEvent * event, QAbstractItemView * view, const QStyleOptionViewItem & option, const QModelIndex & index)
	{
		return false;
	}

	void NotificationViewDelegate::LinkActivated(QString href, const QStyleOptionViewItem & option) const
	{
		auto * view = dynamic_cast<const NotificationView *>(parent());
		if (not view) return;
		
		Q_EMIT view->LinkActivated(std::move(href));
	}

	void NotificationViewDelegate::LinkHovered(QString href, const QStyleOptionViewItem & option) const
	{		
		auto * listView = qobject_cast<const QAbstractItemView *>(option.widget);
		if (href.isEmpty())
			listView->viewport()->unsetCursor();
		else
			listView->viewport()->setCursor(Qt::PointingHandCursor);

		auto * view = dynamic_cast<const NotificationView *>(parent());
		if (not view) return;

		Q_EMIT view->LinkHovered(std::move(href));
	}

	NotificationViewDelegate::NotificationViewDelegate(QObject * parent /* = nullptr */)
		: QAbstractItemDelegate(parent)
	{
		using QtTools::LoadIcon;

		// see https://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
		m_errorIcon = LoadIcon("dialog-error", QStyle::SP_MessageBoxCritical);
		m_warnIcon  = LoadIcon("dialog-warning", QStyle::SP_MessageBoxWarning);
		m_infoIcon  = LoadIcon("dialog-information", QStyle::SP_MessageBoxInformation);
	}

	void NotificationViewDelegate::SearchHighlighter::highlightBlock(const QString & text)
	{
		if (m_searchString.isEmpty()) return;

		int index = text.indexOf(m_searchString, 0, Qt::CaseInsensitive);
		while (index >= 0)
		{
			int length = m_searchString.length();
			setFormat(index, length, m_searchFormat);
			
			index = text.indexOf(m_searchString, index + length, Qt::CaseInsensitive);
		}
	}
}
