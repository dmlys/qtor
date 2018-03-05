#include <array>
#include <memory>
#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <regex>
#include <fstream>

#include <ext/is_string.hpp>
#include <ext/utility.hpp>
#include <ext/base64.hpp>
#include <ext/itoa.hpp>

#include <ext/Errors.hpp>

#include <ext/library_logger/logger.hpp>
#include <ext/library_logger/logging_macros.hpp>

#include <qtor/abstract_data_source.hpp>
#include <qtor/transmission/data_source.hpp>

#include <qtor/torrent_store.hpp>
#include <qtor/TorrentsModel.hpp>

#include <qtor/TorrentsView.hqt>
#include <qtor/MainWindow.hqt>
#include <qtor/Application.hqt>
#include <TransmissionRemoteApp.hqt>

#include <QtWidgets/QApplication>
#include <QtGui/QColor>
#include <QtCore/QDebug>

#include <QtWidgets/QTextEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QShortcut>

//class http_method
//{
//	const char * m_val;
//
//public:
//	constexpr http_method(const char * val) : m_val(val) {}
//	constexpr const char * val() const { return m_val; }
//};
//
//const http_method get     = "get";
//const http_method headers = "headers";
//const http_method put     = "put";
//const http_method delete_ = "delete";
//const http_method post    = "post";
//
//std::array<const char *, 0> empty_headers;
//constexpr char * empty_headers[0] = {};

//template <class Sink, class HeaderString, class ValueString>
//void write_http_header(Sink & sink, const HeaderString & name, const ValueString & value)
//{
//	using ext::netlib::write_string;
//	using ext::netlib::encode_url;
//	
//	write_string(sink, name);
//	write_string(sink, ": ");
//	encode_url(value, sink);
//	write_string(sink, "\r\n");
//}
//
//template <class Sink, class Range>
//std::enable_if_t<ext::is_string_range<Range>::value>
//write_http_headers(Sink & sink, const Range & headers)
//{
//	using std::begin; using std::end;
//	auto first = begin(headers);
//	auto last = end(headers);
//
//	for (;;)
//	{
//		if (first == last) return;
//		auto && name = *first;
//		++first;
//
//		if (first == last) return;
//		auto && val = *first;
//		++first;
//
//		write_http_header(
//			sink,
//			std::forward<decltype(name)>(name),
//			std::forward<decltype(val)>(val)
//		);
//	}
//}
//
//template <class Sink, class Range>
//std::enable_if_t<not ext::is_string_range<Range>::value>
//write_http_headers(Sink & sink, const Range & headers_map)
//{
//	for (auto && entity : headers_map)
//	{
//		using std::get;
//		auto && name = get<0>(entity);
//		auto && val = get<1>(entity);
//
//		write_http_header(
//			sink,
//			std::forward<decltype(name)>(name),
//			std::forward<decltype(val)>(val)
//		);
//	}
//}

#include <qtor/NotificationPopupWidget.hqt>
#include <qtor/NotificationSystem.hqt>
#include <qtor/NotificationSystemExt.hqt>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QDesktopWidget>

class NotificationPopupLabel : public QtTools::NotificationPopupWidget
{
protected:
	QLayout * m_layout = nullptr;
	QLabel * m_label = nullptr;

public:
	QString text() const noexcept { return m_label->text(); }
	void setText(QString text) { m_label->setText(text); }

public:
	NotificationPopupLabel(QWidget * parent = nullptr);
};

NotificationPopupLabel::NotificationPopupLabel(QWidget * parent /* = nullptr */)
	: QtTools::NotificationPopupWidget(parent)
{
	m_label = new QLabel(this);
	m_layout = new QHBoxLayout {this};
	m_layout->addWidget(m_label);
	setLayout(m_layout);

	QColor color = QColor("yellow");
	color.setAlpha(160);
	SetBackgroundBrush(color);
}




class NotificationPopupTest : public QtTools::NotificationPopupWidget
{
protected:	
	QLabel * m_title = nullptr;
	QLabel * m_text = nullptr;
	QLabel * m_timestmap = nullptr;

public:
	NotificationPopupTest(QWidget * parent = nullptr)
		: QtTools::NotificationPopupWidget(parent/*, Qt::ToolTip*/)
	{
		setupUi();
	}


protected:
	void setupUi();
};

void NotificationPopupTest::setupUi()
{
	m_title = new QLabel(this);
	m_timestmap = new QLabel(this);
	m_text = new QLabel(this);

	// word wrapping actually turns heightForWidth support on,
	// if an item on layout supports hasHeightForWidth - layout has it too
	// if widget controlling layout has heightForWidth - widget has it too
	m_title->setWordWrap(true);
	m_timestmap->setWordWrap(true);
	m_text->setWordWrap(true);

	//QFrame * frame = new QFrame(this);
	//frame->setFrameShape(QFrame::HLine);
	//frame->setFrameShadow(QFrame::Plain);
		
	QHBoxLayout * titleLayout = new QHBoxLayout;
	titleLayout->addWidget(m_title);
	titleLayout->addStretch(20);
	titleLayout->addWidget(m_timestmap, 0, Qt::AlignRight);

	QBoxLayout * layout = new QVBoxLayout;
	layout->setSpacing(0);
	layout->setContentsMargins(6, 6 - 4, 6, 6);
	layout->addLayout(titleLayout);
	//layout->addWidget(frame);
	layout->addWidget(m_text);

	setLayout(layout);

	QColor color = QColor("yellow");
	color.setAlpha(200);
	SetBackgroundBrush(color);

	SetShadowColor(Qt::black);

	m_title->setText("Title");
	m_timestmap->setText("2017/05/02");
	m_text->setTextFormat(Qt::RichText);
//	m_text->setText(R"(Your options Are:
//<ol>
//<li>opt 1
//<li>opt 2
//</ol>
//opta hoptra lalalal kilozona <a href = "setings:://tralala" >link</a>
//)");
//)");
	m_text->setText("Error");
}

#include "layout.hqt"

int main(int argc, char * argv[])
{
	using namespace std;
	using namespace qtor;

	ext::winsock2_stream_init();
	ext::init_future_library(std::thread::hardware_concurrency());

	QtTools::QtRegisterStdString();
	QMetaType::registerComparators<datetime_type>();
	QMetaType::registerComparators<duration_type>();

	Q_INIT_RESOURCE(QtTools);
	Q_INIT_RESOURCE(qtor_core_resource);

	QApplication qapp {argc, argv};

//#ifdef Q_OS_WIN
//	// On windows the highlighted colors for inactive widgets are the
//	// same as non highlighted colors.This is a regression from Qt 4.
//	// https://bugreports.qt-project.org/browse/QTBUG-41060
//	auto palette = qapp.palette();
//	palette.setColor(QPalette::Inactive, QPalette::Highlight, palette.color(QPalette::Active, QPalette::Highlight));
//	palette.setColor(QPalette::Inactive, QPalette::HighlightedText, palette.color(QPalette::Active, QPalette::HighlightedText));
//	qapp.setPalette(palette);
//#endif

	//auto source = std::make_shared<qtor::sqlite::sqlite_datasource>();
	//source->set_address("bin/data.db"s);

	//auto source = std::make_shared<qtor::transmission::data_source>();
	//source->set_address("http://melkiy:9091/transmission/rpc"s);
	//
	//qtor::TransmissionRemoteApp app {std::move(source)};
	//qtor::MainWindow mainWindow;
	//
	//mainWindow.Init(app);
	//mainWindow.show();
	//
	//QTimer::singleShot(100, [&app] { app.Connect(); });

	//	auto ttt = R"(Your options Are:
	//<ol>
	//<li>opt 1
	//<li>opt 2
	//</ol>
	//opta hoptra lalalal kilozona <a href = "setings:://tralala" >link</a>
	//)";
	//
	//	std::error_code err {10066, ext::system_utf8_category()};
	//	std::string errmsg = ext::FormatError(err);
	//
	//	QtTools::NotificationSystem::NotificationCenter nsys;
	//	nsys.AddNotification("Title", "Text1");
	//	nsys.AddNotification("Title", "<a href = \"setings:://tralala\">Text2</a>");
	//	nsys.AddNotification("Title", ttt);
	//	nsys.AddNotification("Title", QtTools::ToQString(errmsg));
	//
	//	auto model = nsys.CreateModel();
	//
	//	QtTools::NotificationSystem::NotificationView view;
	//	view.SetModel(model);
	//	view.show();

	QtTools::NotificationSystem::NotificationCenter center;
	QtTools::NotificationSystem::NotificationLayout layout(center);

	auto * notif = new QtTools::NotificationSystem::SimpleNotification;
	notif->Title("TestTitle");
	notif->Text("Test");

	auto * popup = new NotificationPopupTest;

	layout.AddNotification(notif, popup);


	notif = new QtTools::NotificationSystem::SimpleNotification;
	notif->Title("TestTitle2");
	notif->Text("Test2");

	popup = new NotificationPopupTest;

	layout.AddNotification(notif, popup);
	layout.SetCorner(Qt::TopLeftCorner);

	return qapp.exec();
}
