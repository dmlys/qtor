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
#include <QtTools/Delegates/SearchDelegate.hpp>

#include <qtor/NotificationSystem.hqt>
#include <qtor/NotificationSystemExt.hqt>
#include <qtor/NotificationView.hqt>


namespace QtTools::NotificationSystem
{
	const unsigned SimpleNotification::ms_Spacing = 1;
	const QMargins SimpleNotification::ms_ContentMargins = {1, 3, 1, 3};
	const QTextCharFormat SimpleNotification::ms_searchFormat = [] 
	{
		QTextCharFormat format;
		format.setForeground(Qt::GlobalColor::red);
		format.setBackground(Qt::GlobalColor::green);
		return format;
	}();
	
	static const char * ms_OldHrefProperty = "QtTools::NotificationSystem::SimpleNotification::OldHref";
	static const QVariant ms_NullVariant;

	QMargins SimpleNotification::TextMargins(const QStyleOptionViewItem & option)
	{
		return ms_ContentMargins + QtTools::Delegates::TextMargins(option);
	}

	AbstractNotificationPopupWidget * SimpleNotification::CreatePopup() const
	{
		return new SimpleNotificationPopup(*this);
	}

	void SimpleNotification::PrepareTextDocument(QTextDocument & textDoc, const LaidoutItem & item)
	{
		QPaintDevice * device = const_cast<QWidget *>(item.option->widget);

		textDoc.setDefaultTextOption(QtTools::Delegates::TextLayout::PrepareTextOption(*item.option));
		textDoc.setDocumentMargin(0); // be default it's == 4
		textDoc.setDefaultFont(item.textFont);
		textDoc.documentLayout()->setPaintDevice(device);
		textDoc.setHtml(item.text);
	}

	void SimpleNotification::LayoutItem(const QStyleOptionViewItem & option, LaidoutItem & item) const 
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

		auto * model = dynamic_cast<const AbstractNotificationModel *>(option.index.model());
		if (model) item.searchStr = model->GetFilter();

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
		auto * highlighter = new SearchHighlighter(item.textdocptr.get());
		highlighter->SetSearchText(item.searchStr);
		highlighter->SetFormat(ms_searchFormat);

		QTextDocument & textDoc = *item.textdocptr;
		PrepareTextDocument(textDoc, item);

		const auto width = std::max(rect.width(), titleSz.width() + timestampSz.width() + titleSpacer);
		textDoc.setTextWidth(width);

		QSize textSz {std::lround((textDoc.idealWidth())), std::lround(textDoc.size().height())};
		int textTop = ms_Spacing + std::max(item.titleRect.bottom(), item.timestampRect.bottom());
		item.textRect = QRect {{topLeft.x(), textTop}, textSz};

		item.totalRect = item.timestampRect | item.titleRect | item.textRect;
		item.totalRect += margins;
		
		return;
	}

	void SimpleNotification::Draw(QPainter * painter, const LaidoutItem & item) const
	{
		using namespace QtTools::Delegates;

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
		//DrawFormattedText(painter, item.title, item.titleRect, titleopt);
		DrawSearchFormatedText(painter, item.title, item.titleRect, titleopt, 
		                       FormatSearchText(item.title, item.searchStr, ms_searchFormat));

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

	void SimpleNotification::DrawBackground(QPainter * painter, const LaidoutItem & item) const
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

	void SimpleNotification::paint(QPainter * painter, const QStyleOptionViewItem & option, std::any & cookie) const
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

	QSize SimpleNotification::sizeHint(const QStyleOptionViewItem & option, std::any & cookie) const
	{
		auto * cache = std::any_cast<LaidoutItem>(&cookie);
		if (not cache) cache = &cookie.emplace<LaidoutItem>();

		// sizeHint should always be recalculated
		// force it by setting invalid index
		cache->notification = nullptr;

		LayoutItem(option, *cache);
		return cache->totalRect.size();
	}

	bool SimpleNotification::editorEvent(QEvent * event, const QStyleOptionViewItem & option, std::any & cookie) const
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

	void SimpleNotification::LinkActivated(QString href, const QStyleOptionViewItem & option) const
	{
		auto * view = dynamic_cast<const NotificationView *>(option.widget);
		if (not view) return;
		
		Q_EMIT view->LinkActivated(std::move(href));
	}

	void SimpleNotification::LinkHovered(QString href, const QStyleOptionViewItem & option) const
	{		
		auto * listView = qobject_cast<const QAbstractItemView *>(option.widget);
		listView->viewport()->setCursor(Qt::PointingHandCursor);

		auto * view = dynamic_cast<const NotificationView *>(option.widget);
		if (not view) return;

		Q_EMIT view->LinkHovered(std::move(href));
	}

	void SimpleNotification::SearchHighlighter::highlightBlock(const QString & text)
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

	/************************************************************************/
	/*                   SimpleNotificationPopup                            */
	/************************************************************************/
	auto AbstractNotificationPopupWidget::GetNotificationCenter() const -> NotificationCenter *
	{
		return m_center;
	}

	void AbstractNotificationPopupWidget::SetNotificationCenter(NotificationCenter * center)
	{
		m_center = center;
		Q_EMIT NotificationCenterChanged(center);
	}

	SimpleNotificationPopup::SimpleNotificationPopup(const SimpleNotification & notification, QWidget * parent /* = nullptr */)
		: AbstractNotificationPopupWidget(parent, Qt::ToolTip)
	{
		m_notification = &notification;
		setupUi();
	}

	void SimpleNotificationPopup::SetNotificationCenter(NotificationCenter * center)
	{
		if (m_center) m_center->disconnect(this);

		AbstractNotificationPopupWidget::SetNotificationCenter(center);
	}
	
	void SimpleNotificationPopup::setupUi()
	{
		QColor color = QColor("yellow");
		color.setAlpha(200);
		SetBackgroundBrush(color);
		SetShadowColor(Qt::black);


		m_title     = new QLabel(this);
		m_timestamp = new QLabel(this);
		m_text      = new QLabel(this);

		// word wrapping actually turns heightForWidth support on,
		// if an item on layout supports hasHeightForWidth - layout has it too
		// if widget controlling layout has heightForWidth - widget has it too
		m_title->setWordWrap(true);
		//m_timestamp->setWordWrap(true);
		m_text->setWordWrap(true);

		auto titleFont = this->font();
		titleFont.setPointSize(titleFont.pointSize() * 11 / 10);
		titleFont.setBold(true);

		m_timestamp->setFont(titleFont);
		m_title->setFont(titleFont);

		auto loc = locale();
		auto timestamp = loc.toString(m_notification->Timestamp());
		auto title = m_notification->Title();
		auto text = m_notification->Text();

		m_title->setText(title);
		m_timestamp->setText(timestamp);
		
		m_text->setTextFormat(Qt::RichText);
		m_text->setText(text);


		QHBoxLayout * titleLayout = new QHBoxLayout;
		titleLayout->addWidget(m_title, 1);
		titleLayout->addSpacing(20);
		titleLayout->addWidget(m_timestamp, 0, Qt::AlignRight);

		QBoxLayout * layout = new QVBoxLayout;
		layout->setSpacing(0);
		layout->setContentsMargins(6, 6 - 4, 6, 6);
		layout->addLayout(titleLayout);
		layout->addWidget(m_text);

		setLayout(layout);
	}

	/************************************************************************/
	/*                   SimpleNotificationDelegate                         */
	/************************************************************************/
	void SimpleNotificationDelegate::init(const QStyleOptionViewItem & option, const QModelIndex & index) const
	{
		auto & opt = ext::unconst(option);
		opt.index = index;
	}

	void SimpleNotificationDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
	{
		init(option, index);

		auto * model = dynamic_cast<const AbstractNotificationModel *>(index.model());
		const auto * notification = model->GetItem(index.row());
		return notification->paint(painter, option, m_cachedItem);
	}

	QSize SimpleNotificationDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
	{
		init(option, index);

		auto * model = dynamic_cast<const AbstractNotificationModel *>(index.model());
		const auto * notification = model->GetItem(index.row());
		return notification->sizeHint(option, m_cachedItem);
	}

	bool SimpleNotificationDelegate::editorEvent(QEvent * event, QAbstractItemModel *, const QStyleOptionViewItem & option, const QModelIndex & index)
	{
		init(option, index);

		auto * model = dynamic_cast<const AbstractNotificationModel *>(index.model());
		const auto * notification = model->GetItem(index.row());
		return notification->editorEvent(event, option, m_cachedItem);
	}
}
