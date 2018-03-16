#include <QtCore/QLocale>
#include <QtWidgets/QLabel>
#include <QtWidgets/QBoxLayout>

#include <qtor/NotificationSystem.hqt>
#include <qtor/NotificationLayout.hqt>
#include <qtor/NotificationLayoutExt.hqt>

namespace QtTools::NotificationSystem
{
	NotificationPopupWidget::NotificationPopupWidget(const Notification & notification, QWidget * parent /* = nullptr */)
		: AbstractNotificationPopupWidget(parent, Qt::ToolTip)
	{
		m_notification = &notification;
		setupUi();
		connectSignals();
	}
	
	void NotificationPopupWidget::setupUi()
	{
		QColor color = QColor("yellow");
		color.setAlpha(200);
		SetBackgroundBrush(color);
		SetShadowColor(Qt::black);


		m_title =     new QLabel(this);
		m_timestamp = new QLabel(this);
		m_text =      new QLabel(this);

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
		auto tfmt = m_notification->TextFmt();

		m_title->setText(title);
		m_timestamp->setText(timestamp);

		m_text->setTextFormat(tfmt);
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

	void NotificationPopupWidget::connectSignals()
	{
		connect(m_text, &QLabel::linkActivated, this, &NotificationPopupWidget::LinkActivated);
		connect(m_text, &QLabel::linkHovered, this, &NotificationPopupWidget::LinkHovered);
	}
}
