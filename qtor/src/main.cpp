#include <array>
#include <memory>
#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <regex>
#include <fstream>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <ext/is_string.hpp>
#include <ext/utility.hpp>
#include <ext/base64.hpp>
#include <ext/itoa.hpp>

#include <ext/Errors.hpp>
#include <ext/range/pretty_printers.hpp>

#include <ext/library_logger/logger.hpp>
#include <ext/library_logger/logging_macros.hpp>

#include <qtor/abstract_data_source.hpp>
#include <qtor/transmission/data_source.hpp>

#include <qtor/torrent_store.hpp>
#include <qtor/torrent_file_store.hpp>
#include <qtor/TorrentsModel.hpp>

#include <qtor/TorrentsView.hqt>
#include <qtor/MainWindow.hqt>
#include <qtor/Application.hqt>
#include "TransmissionRemoteApp.hqt"

#include "ScreenInfo.hpp"
#include <qtor/NotificationSystem.hqt>
#include <qtor/NotificationView.hqt>
#include <qtor/NotificationPopupLayout.hqt>

#include <QtGui/QPainter>
#include <QtGui/QTextDocument>
#include <QtGui/QTextBlock>
#include <QtGui/QAbstractTextDocumentLayout>
#include <QtGui/QTextCursor>
#include <QtTools/ItemViewUtils.hpp>

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

#include <qtor/FileTreeModel.hqt>
#include <qtor/FileTreeView.hqt>


int main(int argc, char * argv[])
{
	using namespace std;
	using namespace qtor;

	ext::netlib::socket_stream_init();
	ext::init_future_library(std::thread::hardware_concurrency());

	QtTools::QtRegisterStdString();
	QMetaType::registerComparators<datetime_type>();
	QMetaType::registerComparators<duration_type>();

	Q_INIT_RESOURCE(QtTools);
	Q_INIT_RESOURCE(qtor_core_resource);

	QApplication qapp {argc, argv};

	std::cout << QtTools::ScreenInfo << endl;

	std::vector<torrent_file> paths, paths2;
	paths.assign({
	    { QStringLiteral("folder/file1.txt"), 100 * 100, 20 * 100 },
		{ QStringLiteral("folder/file1.txt"), 100 * 100, 20 * 100 },
	    { QStringLiteral("folder/file2.txt"), 100 * 100, 20 * 100 },
	    { QStringLiteral("dir/file.sft"),     100 * 100, 20 * 100 },
	    { QStringLiteral("dir/prox/dir.txt"),     100 * 100, 20 * 100 },
	    { QStringLiteral("ops.sh"),           100 * 100, 20 * 100 },
	    { QStringLiteral("westworld.mkv"),    100 * 100, 20 * 100 },
	    { QStringLiteral("folder/sup/file3.txt"), 100 * 100, 20 * 100 },
	    { QStringLiteral("folder/sup/inner/file.txt"), 100 * 100, 20 * 100 },
	});

	paths2 = paths;
	paths2.back().filename = QStringLiteral("upsershalt/ziggaman.txt");
	
	//paths.assign({
	//	{ QStringLiteral("file1.txt") },
	//	{ QStringLiteral("file2.txt") },
	//	{ QStringLiteral("file.sft") },
	//	{ QStringLiteral("file.txt") },
	//	{ QStringLiteral("ops.sh") },
	//	{ QStringLiteral("westworld.mkv") },
	//});

	//paths2.assign({
	//		{QStringLiteral("file1.txt")},
	//		{QStringLiteral("file.sft")},
	//		{QStringLiteral("ziggaman.sh")},
	//		{QStringLiteral("westworld.mkv")},
	//});

	auto store = std::make_shared<torrent_file_store>();
	auto model = std::make_shared<FileTreeModel>();
	//auto model = std::make_shared<FileTreeModel>(store);
	
	//store->assign_records(paths);
	//store->assign_records(paths2);
	//store->assign_records(paths);

	model->assign(paths);
	model->upsert(paths2);
	model->assign(paths);
	
	//QTableView view;
	//view.setModel(model.get());
	FileTreeView view;
	view.SetModel(model);
	QtTools::ResizeColumnsToContents(view.GetTreeView());

	//QTimer::singleShot(3s, [&view] {
	    view.showMaximized();
		view.activateWindow();
		view.raise();
	//});
	



//#ifdef Q_OS_WIN
//	// On windows the highlighted colors for inactive widgets are the
//	// same as non highlighted colors.This is a regression from Qt 4.
//	// https://bugreports.qt-project.org/browse/QTBUG-41060
//	auto palette = qapp.palette();
//	palette.setColor(QPalette::Inactive, QPalette::Highlight, palette.color(QPalette::Active, QPalette::Highlight));
//	palette.setColor(QPalette::Inactive, QPalette::HighlightedText, palette.color(QPalette::Active, QPalette::HighlightedText));
//	qapp.setPalette(palette);
//#endif

////	auto source = std::make_shared<qtor::sqlite::sqlite_datasource>();
////	source->set_address("bin/data.db"s);

//	auto source = std::make_shared<qtor::transmission::data_source>();
//	source->set_address("http://melkiy:9091/transmission/rpc"s);

//	qtor::TransmissionRemoteApp app {std::move(source)};
//	qtor::MainWindow mainWindow;

//	mainWindow.Init(app);
//	mainWindow.show();

//	QTimer::singleShot(100, [&app] { app.Connect(); });
//	qapp.setQuitOnLastWindowClosed(true);

//	auto longTitle = "Some Very Long Title, No, Seriosly, Seriosly, Seriosly, And this quiet pricnce should not be seen. Even longer than you think, forget it";

//	std::error_code err {EACCES, std::generic_category()};
//	std::string errmsg = ext::FormatError(err);

//	using namespace QtTools::NotificationSystem;

//	NotificationCenter nsys;
//	NotificationPopupLayout layout;
//	NotificationView view;

//	auto hovered = [bar = mainWindow.m_statusbar](auto href) { bar->showMessage(href); };
//	auto activated = [](auto href) { cout << FromQString(href) << endl; };

//	QObject::connect(&layout, &NotificationPopupLayout::LinkHovered, mainWindow.m_statusbar, hovered);
//	QObject::connect(&view, &NotificationView::LinkHovered, mainWindow.m_statusbar, hovered);

//	QObject::connect(&layout, &NotificationPopupLayout::LinkActivated, activated);
//	QObject::connect(&view, &NotificationView::LinkActivated, activated);

//	layout.Init(nsys);
//	layout.SetParent(nullptr);
//	//layout.SetCorner(Qt::TopRightCorner);
//	//layout.SetExpirationTimeouts(600ms, 400ms, 200ms);

//	view.Init(nsys);
//	view.SetFilterMode(view.FilterByText | view.FilterByLevel);
//	view.show();

//	nsys.AddInfo("Title1", "Text1");
//	nsys.AddInfo("Title2", "<a href = \"setings:://tralala\">Text2</a>");
//	nsys.AddError("Title4", QtTools::ToQString(errmsg));
//	nsys.AddWarning("Title5", QtTools::ToQString(errmsg));
//	nsys.AddInfo("Title6", QtTools::ToQString(errmsg));
//	nsys.AddInfo("Title7", QtTools::ToQString(errmsg));
//	nsys.AddInfo(longTitle, QtTools::ToQString(errmsg));

//	auto nf = nsys.CreateNotification();
//	nf->Title("Custom");
//	nf->Text("Some Text");
//	nf->setProperty("backgroundColor", QColor("red"));
//	nf->setProperty("expirationTimeout", 0);
//	nf->ActivationLink("setings:://tralala");

//	nsys.AddNotification(std::move(nf));

	return qapp.exec();
}
