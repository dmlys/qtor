#include <QtTools/Delegates/Utils.hpp>
#include <QtTools/Delegates/DrawFormattedText.hpp>
#include <qtor/NotificationSystem.hpp>

namespace QtTools
{
	const QMargins NotificationSystem::SimpleNotification::ms_InnerMargins = {0, 1, 0, 1};
	const QMargins NotificationSystem::SimpleNotification::ms_OutterMargins = {4, 4, 4, 4};

	NotificationSystem::SimpleNotificationDelegate::SimpleNotificationDelegate(QObject * parent /* = nullptr */)
		: QAbstractItemDelegate(parent)
	{

	}

	void NotificationSystem::SimpleNotificationDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
	{
		
	}

	QSize NotificationSystem::SimpleNotificationDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
	{

	}

	auto NotificationSystem::SimpleNotification::LayoutItem(const QStyleOptionViewItem & option, const QModelIndex & index) const 
		-> LaidoutItem
	{
		LaidoutItem item;
		item.option = &option;
		//if (item.index == index)
		//{
		//	auto newTopLeft = option.rect.topLeft();
		//	auto diff = option.rect.topLeft() - item.hintTopLeft;
		//	item.hintTopLeft = option.rect.topLeft();

		//	item.titleRect.translate(diff);
		//	item.datetimeRect.translate(diff);
		//	item.textRect.translate(diff);
		//	item.iconRect.translate(diff);

		//	return;
		//}

		item.hintTopLeft = option.rect.topLeft();
		item.index = index;

		item.timestamp = m_timestamp.toString();
		item.title = m_title;
		item.text = m_text;
		item.icon = m_icon;

		item.baseFont = option.font;
		item.titleFont = option.font;
		item.textFont = option.font;

		QFontMetrics fm {item.titleFont};
		auto topLeft = option.rect.topLeft();

		auto elided = QtTools::Delegates::TextLayout::ElideText(fm, item.title, option.textElideMode, option.rect.width());
	}

	void NotificationSystem::SimpleNotification::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
	{

	}

	QSize NotificationSystem::SimpleNotification::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
	{

	}
}
