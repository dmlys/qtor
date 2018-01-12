#include <QtGui/QPainter>
#include <QtTools/Delegates/Utils.hpp>
#include <QtTools/Delegates/StyledParts.hpp>
#include <QtTools/Delegates/DrawFormattedText.hpp>

#include <qtor/NotificationSystem.hqt>
#include <qtor/NotificationSystemExt.hqt>


namespace QtTools
{
	const QMargins NotificationSystem::SimpleNotification::ms_InnerMargins = {0, 1, 0, 1};
	const QMargins NotificationSystem::SimpleNotification::ms_OutterMargins = {4, 4, 4, 4};	

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

	void NotificationSystem::SimpleNotification::LayoutItem(const QStyleOptionViewItem & option, LaidoutItem & item) const 
	{
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
		
		QPaintDevice * device = const_cast<QWidget *>(option.widget);

		item.notification = this;
		item.hintTopLeft = option.rect.topLeft();
		item.timestamp = m_timestamp.toString(Qt::DateFormat::DefaultLocaleShortDate);
		item.title = m_title;
		item.text = m_text;
		item.pixmap = m_pixmap;

		item.textFont = item.titleFont = item.baseFont = option.font;
		item.titleFont.setPointSize(item.baseFont.pointSize() * 9 / 10);
		item.titleFont.setBold(true);

		
		auto rect = option.rect - ms_OutterMargins;
		auto topLeft = rect.topLeft();

		item.pixmapRect = QRect {topLeft, item.pixmap.size()} - ms_InnerMargins;		
		auto textLeft = item.pixmapRect.right() + ms_InnerMargins.left();

		QFontMetrics titleFm {item.titleFont, device};
		QFontMetrics textFm {item.textFont, device};
		QSize timestampSize(titleFm.width(item.timestamp), titleFm.height());

		auto left = std::max(textLeft, rect.right() - timestampSize.width());
		item.timestampRect = QRect {left, rect.top(), rect.right() - left, rect.top() + timestampSize.height()};
		item.timestampRect -= ms_InnerMargins;

		item.titleRect = QRect {textLeft, rect.top(), left, rect.top() + timestampSize.height()};
		item.titleRect -= ms_InnerMargins;

		item.textRect = QRect {textLeft, rect.top() + timestampSize.height(), rect.right(), rect.bottom()};
		item.textRect -= ms_InnerMargins;
		item.textRect = textFm.boundingRect(item.textRect, Qt::AlignLeft | Qt::TextWordWrap, item.text);

		item.totalRect = item.pixmapRect | item.timestampRect | item.textRect;
		item.totalRect += ms_InnerMargins;
		item.totalRect += ms_OutterMargins;
	}

	void NotificationSystem::SimpleNotification::Draw(QPainter * painter, const LaidoutItem & item) const
	{
		painter->drawPixmap(item.pixmapRect, item.pixmap);

		const QStyleOptionViewItem & option = *item.option;
		const bool selected = option.state & QStyle::State_Selected;

		auto cg = QtTools::Delegates::ColorGroup(option);
		auto cr = selected ? QPalette::HighlightedText : QPalette::Text;

		painter->setPen(option.palette.color(cg, cr));

		painter->setFont(item.titleFont);
		painter->drawText(item.titleRect, item.text);
		painter->drawText(item.timestampRect, item.timestamp);

		painter->setFont(item.textFont);
		painter->drawText(item.textRect, Qt::AlignLeft, item.text);
	}

	void NotificationSystem::SimpleNotification::DrawBackground(QPainter * painter, const LaidoutItem & item) const
	{
		using QtTools::Delegates::ColorGroup;

		const QStyleOptionViewItem & option = *item.option;
		const bool selected = option.state & QStyle::State_Selected;

		if (selected)
		{
			auto cg = ColorGroup(option);
			painter->fillRect(item.option->rect, option.palette.brush(cg, QPalette::Highlight));
		}

		using namespace QtTools::Delegates;
		if (HasFocusFrame(option))
		{
			DrawFocusFrame(painter, item.option->rect, option);
		}
	}

	void NotificationSystem::SimpleNotification::paint(QPainter * painter, const QStyleOptionViewItem & option, std::any & cookie) const
	{
		auto * cache = std::any_cast<LaidoutItem>(&cookie);
		if (not cache) cache = &cookie.emplace<LaidoutItem>();

		LayoutItem(option, *cache);

		painter->save();
		DrawBackground(painter, *cache);
		Draw(painter, *cache);
		painter->restore();
	}

	QSize NotificationSystem::SimpleNotification::sizeHint(const QStyleOptionViewItem & option, std::any & cookie) const
	{
		auto * cache = std::any_cast<LaidoutItem>(&cookie);
		if (not cache) cache = &cookie.emplace<LaidoutItem>();
		
		LayoutItem(option, *cache);
		return cache->totalRect.size();

	}

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
}
