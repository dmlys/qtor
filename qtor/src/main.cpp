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


#include <qtor/NotificationSystem.hqt>
#include <qtor/NotificationView.hqt>
#include <qtor/NotificationLayout.hqt>

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

#include <QtCore/QDebug>
#include <QtWidgets/QDesktopWidget>
#include <QtGui/QScreen>

#include <Windows.h>

// Helper function to return display orientation as a string.
QString Orientation(Qt::ScreenOrientation orientation)
{
	switch (orientation) {
		case Qt::PrimaryOrientation: return "Primary";
		case Qt::LandscapeOrientation: return "Landscape";
		case Qt::PortraitOrientation: return "Portrait";
		case Qt::InvertedLandscapeOrientation: return "Inverted landscape";
		case Qt::InvertedPortraitOrientation: return "Inverted portrait";
		default: return "Unknown";
	}
}

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

	//auto * widget = qapp.desktop()->screen();
	//auto * screen = qapp.primaryScreen();

	HDC hdc = GetDC(nullptr);

	int hz = GetDeviceCaps(hdc, HORZSIZE);
	int vz = GetDeviceCaps(hdc, VERTSIZE);

	qDebug() << hz << "x" << vz;

	ReleaseDC(nullptr, hdc);

	qDebug() << "Number of screens:" << QGuiApplication::screens().size();

	qDebug() << "Primary screen:" << QGuiApplication::primaryScreen()->name();

	foreach(QScreen *screen, QGuiApplication::screens())
	{
		qDebug() << "Information for screen:" << screen->name();
		qDebug() << "  Available geometry:" << screen->availableGeometry().x() << screen->availableGeometry().y() << screen->availableGeometry().width() << "x" << screen->availableGeometry().height();
		qDebug() << "  Available size:" << screen->availableSize().width() << "x" << screen->availableSize().height();
		qDebug() << "  Available virtual geometry:" << screen->availableVirtualGeometry().x() << screen->availableVirtualGeometry().y() << screen->availableVirtualGeometry().width() << "x" << screen->availableVirtualGeometry().height();
		qDebug() << "  Available virtual size:" << screen->availableVirtualSize().width() << "x" << screen->availableVirtualSize().height();
		qDebug() << "  Depth:" << screen->depth() << "bits";
		qDebug() << "  Geometry:" << screen->geometry().x() << screen->geometry().y() << screen->geometry().width() << "x" << screen->geometry().height();
		qDebug() << "  Logical DPI:" << screen->logicalDotsPerInch();
		qDebug() << "  Logical DPI X:" << screen->logicalDotsPerInchX();
		qDebug() << "  Logical DPI Y:" << screen->logicalDotsPerInchY();
		qDebug() << "  Orientation:" << Orientation(screen->orientation());
		qDebug() << "  Physical DPI:" << screen->physicalDotsPerInch();
		qDebug() << "  Physical DPI X:" << screen->physicalDotsPerInchX();
		qDebug() << "  Physical DPI Y:" << screen->physicalDotsPerInchY();
		qDebug() << "  Physical size:" << screen->physicalSize().width() << "x" << screen->physicalSize().height() << "mm";
		qDebug() << "  Primary orientation:" << Orientation(screen->primaryOrientation());
		qDebug() << "  Refresh rate:" << screen->refreshRate() << "Hz";
		qDebug() << "  Size:" << screen->size().width() << "x" << screen->size().height();
		qDebug() << "  Virtual geometry:" << screen->virtualGeometry().x() << screen->virtualGeometry().y() << screen->virtualGeometry().width() << "x" << screen->virtualGeometry().height();
		qDebug() << "  Virtual size:" << screen->virtualSize().width() << "x" << screen->virtualSize().height();
	}



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

	auto ttt = R"(Your options Are:
<ol>
<li>opt 1
<li>opt 2
</ol>
opta hoptra lalalal kilozona <a href = "setings:://tralala" >link</a>
)";
	
	std::error_code err {10066, ext::system_utf8_category()};
	std::string errmsg = ext::FormatError(err);
	
	using namespace QtTools::NotificationSystem;

	QtTools::NotificationSystem::NotificationCenter nsys;
	QtTools::NotificationSystem::NotificationLayout layout;
	QtTools::NotificationSystem::NotificationView view;
	
	layout.Init(nsys);
	//layout.SetCorner(Qt::TopRightCorner);

	view.Init(nsys);
	view.show();

	nsys.AddNotification("Title", "Text1");
	nsys.AddNotification("Title", "<a href = \"setings:://tralala\">Text2</a>");
	nsys.AddNotification("Title", ttt, Qt::RichText);
	nsys.AddNotification("Title", QtTools::ToQString(errmsg));
	nsys.AddNotification("Title", QtTools::ToQString(errmsg));
	nsys.AddNotification("Title", QtTools::ToQString(errmsg));
	nsys.AddNotification("Title", QtTools::ToQString(errmsg));

	return qapp.exec();
}
