////#include <array>
////#include <memory>
////#include <string>
////#include <map>
////#include <iostream>
////#include <sstream>
////#include <stdexcept>
////#include <regex>
////#include <fstream>

////#include <fmt/format.h>
////#include <fmt/ostream.h>

////#include <ext/is_string.hpp>
////#include <ext/strings/cow_string.hpp>
////#include <ext/utility.hpp>
////#include <ext/base64.hpp>
////#include <ext/itoa.hpp>

////#include <ext/Errors.hpp>
////#include <ext/range/pretty_printers.hpp>

////#include <ext/library_logger/logger.hpp>
////#include <ext/library_logger/logging_macros.hpp>

////#include <qtor/abstract_data_source.hpp>
////#include <qtor/transmission/data_source.hpp>

////#include <qtor/torrent_store.hpp>
////#include <qtor/torrent_file_store.hpp>
////#include <qtor/TorrentsModel.hpp>

////#include <qtor/TorrentsView.hqt>
////#include <qtor/MainWindow.hqt>
////#include <qtor/Application.hqt>
////#include "TransmissionRemoteApp.hqt"

////#include "ScreenInfo.hpp"
////#include <QtTools/NotificationSystem/NotificationSystem.hqt>
////#include <QtTools/NotificationSystem/NotificationView.hqt>
////#include <QtTools/NotificationSystem/NotificationPopupLayout.hqt>

////#include <QtGui/QPainter>
////#include <QtGui/QTextDocument>
////#include <QtGui/QTextBlock>
////#include <QtGui/QAbstractTextDocumentLayout>
////#include <QtGui/QTextCursor>
////#include <QtTools/ItemViewUtils.hpp>
////#include <QtTools/ListModel.hqt>


//////class http_method
//////{
//////	const char * m_val;
//////
//////public:
//////	constexpr http_method(const char * val) : m_val(val) {}
//////	constexpr const char * val() const { return m_val; }
//////};
//////
//////const http_method get     = "get";
//////const http_method headers = "headers";
//////const http_method put     = "put";
//////const http_method delete_ = "delete";
//////const http_method post    = "post";
//////
//////std::array<const char *, 0> empty_headers;
//////constexpr char * empty_headers[0] = {};

//////template <class Sink, class HeaderString, class ValueString>
//////void write_http_header(Sink & sink, const HeaderString & name, const ValueString & value)
//////{
//////	using ext::netlib::write_string;
//////	using ext::netlib::encode_url;
//////
//////	write_string(sink, name);
//////	write_string(sink, ": ");
//////	encode_url(value, sink);
//////	write_string(sink, "\r\n");
//////}
//////
//////template <class Sink, class Range>
//////std::enable_if_t<ext::is_string_range<Range>::value>
//////write_http_headers(Sink & sink, const Range & headers)
//////{
//////	using std::begin; using std::end;
//////	auto first = begin(headers);
//////	auto last = end(headers);
//////
//////	for (;;)
//////	{
//////		if (first == last) return;
//////		auto && name = *first;
//////		++first;
//////
//////		if (first == last) return;
//////		auto && val = *first;
//////		++first;
//////
//////		write_http_header(
//////			sink,
//////			std::forward<decltype(name)>(name),
//////			std::forward<decltype(val)>(val)
//////		);
//////	}
//////}
//////
//////template <class Sink, class Range>
//////std::enable_if_t<not ext::is_string_range<Range>::value>
//////write_http_headers(Sink & sink, const Range & headers_map)
//////{
//////	for (auto && entity : headers_map)
//////	{
//////		using std::get;
//////		auto && name = get<0>(entity);
//////		auto && val = get<1>(entity);
//////
//////		write_http_header(
//////			sink,
//////			std::forward<decltype(name)>(name),
//////			std::forward<decltype(val)>(val)
//////		);
//////	}
//////}

////#include <qtor/FileTreeModel.hqt>
////#include <qtor/FileTreeView.hqt>
////#include <QtTools/PlainLabel.hqt>
////#include <QtTools/DateUtils.hpp>

////#include <QtXml/QtXml>
////#include <QtXmlPatterns/QXmlFormatter>

////#include <qtor/qt_model_meta.hpp>

////#include <QtWidgets/QStyleFactory>
////#include <QtWidgets/QMessageBox>

////#include <ext/netlib/listener.hpp>

////int main(int argc, char * argv[])
////{
////	using namespace std;
////	using namespace qtor;

////	QtTools::QtRegisterStdString();
////	QtTools::QtRegisterStdChronoTypes();

////	ext::netlib::socket_stream_init();
////	ext::init_future_library(std::thread::hardware_concurrency());

////	ext::netlib::listener list;
////	list.bind(8080);
////	list.listen();

////	ext::netlib::socket_streambuf ss;
////	ss.connect("localhost", 8080);

////	auto recv = list.accept();

////	char buffer[1024];
////	std::fill_n(buffer, 1024, '1');

////	unsigned u = 0;
////	for (;;)
////	{
////		fmt::print("step = {}\n", u++);
////		ss.sputn(buffer, 1024);
////	}

////	return 0;

////	Q_INIT_RESOURCE(QtTools);
////	Q_INIT_RESOURCE(qtor_core_resource);

////	auto source = std::make_shared<qtor::sqlite::sqlite_datasource>();
////	source->set_address("bin/data.db"s);

////	//auto source = std::make_shared<qtor::transmission::data_source>();
////	//source->set_address("http://melkiy:9091/transmission/rpc"s);

////	QApplication qapp {argc, argv};

////	source->connect().get();
////	auto files = source->get_torrent_files("174").get();

//////	std::vector<torrent_file> paths, paths2;
//////	paths.assign({
//////	    { QStringLiteral("folder/file1.txt"), 11 * 1000, 2 * 1000 },
//////	    { QStringLiteral("folder/file1.txt"), 12 * 1000, 3 * 1000 },
//////	    { QStringLiteral("folder/file2.txt"), 13 * 1000, 4 * 1000 },
//////	    { QStringLiteral("dir/file.sft"),     14 * 1000, 5 * 1000 },
//////	    { QStringLiteral("dir/prox/dir.txt"), 15 * 1000, 6 * 1000 },
//////	    { QStringLiteral("ops.sh"),           16 * 1000, 7 * 1000 },
//////	    { QStringLiteral("westworld.mkv"),    17 * 1000, 8 * 1000 },
//////	    { QStringLiteral("folder/sup/file3.txt"), 10 * 1000, 2 * 1000 },
//////	    { QStringLiteral("folder/sup/inner/file.txt"), 10 * 1000, 2 * 1000 },
//////	});

//////	paths2 = paths;
//////	paths2.back().filename = QStringLiteral("upsershalt/ziggaman.txt");
	
////	//paths.assign({
////	//	{ QStringLiteral("file1.txt") },
////	//	{ QStringLiteral("file2.txt") },
////	//	{ QStringLiteral("file.sft") },
////	//	{ QStringLiteral("file.txt") },
////	//	{ QStringLiteral("ops.sh") },
////	//	{ QStringLiteral("westworld.mkv") },
////	//});

////	//paths2.assign({
////	//		{QStringLiteral("file1.txt")},
////	//		{QStringLiteral("file.sft")},
////	//		{QStringLiteral("ziggaman.sh")},
////	//		{QStringLiteral("westworld.mkv")},
////	//});

////	//auto store = std::make_shared<torrent_file_store>();
////	auto model = std::make_shared<FileTreeModel>();
////	//auto model = std::make_shared<FileTreeModel>(store);
	
//////	//store->assign_records(paths);
//////	//store->assign_records(paths2);
//////	//store->assign_records(paths);

////	model->assign(files);
//////	model->assign(paths);
//////	model->upsert(paths2);
//////	model->assign(paths);
	
////	auto idx0 = model->find_element("dir1");
////	Q_ASSERT(not idx0.isValid());
////	auto idx1 = model->find_element("dir");
////	auto idx2 = model->find_element("dir/prox");
////	auto idx3 = model->find_element("dir/prox/dir.txt");

////	//QTableView view;
////	//view.setModel(model.get());
////	FileTreeView view;
////	view.SetModel(model);
////	QtTools::ResizeColumnsToContents(view.GetTreeView());

////	view.GetTreeView()->expand(idx1);
////	view.GetTreeView()->expand(idx2);
////	view.GetTreeView()->selectionModel()->select(idx3, QItemSelectionModel::Select);
////	//view.GetTreeView()->setRootIndex(idx1);
////	//QTimer::singleShot(3s, [&view] {
////	    view.showMaximized();
////		    view.activateWindow();
////		view.raise();
////	//});
	



////#ifdef Q_OS_WIN
////	// On windows the highlighted colors for inactive widgets are the
////	// same as non highlighted colors.This is a regression from Qt 4.
////	// https://bugreports.qt-project.org/browse/QTBUG-41060
////	auto palette = qapp.palette();
////	palette.setColor(QPalette::Inactive, QPalette::Highlight, palette.color(QPalette::Active, QPalette::Highlight));
////	palette.setColor(QPalette::Inactive, QPalette::HighlightedText, palette.color(QPalette::Active, QPalette::HighlightedText));
////	qapp.setPalette(palette);
////#endif

//////	qtor::TransmissionRemoteApp app {std::move(source)};
//////	qtor::MainWindow mainWindow;

//////	mainWindow.Init(app);
//////	mainWindow.show();

//////	QTimer::singleShot(100, [&app] { app.Connect(); });

//////	auto longTitle = "Some Very Long Title, No, Seriosly, Seriosly, Seriosly, And this quiet pricnce should not be seen. Even longer than you think, forget it";

//////	std::error_code err {EACCES, std::generic_category()};
//////	std::string errmsg = ext::FormatError(err);

//////	using namespace QtTools::NotificationSystem;

//////	NotificationCenter nsys;
//////	NotificationPopupLayout layout;
//////	NotificationView view;

//////	auto hovered = [bar = mainWindow.m_statusbar](auto href) { bar->showMessage(href); };
//////	auto activated = [](auto href) { cout << FromQString(href) << endl; };

//////	QObject::connect(&layout, &NotificationPopupLayout::LinkHovered, mainWindow.m_statusbar, hovered);
//////	QObject::connect(&view, &NotificationView::LinkHovered, mainWindow.m_statusbar, hovered);

//////	QObject::connect(&layout, &NotificationPopupLayout::LinkActivated, activated);
//////	QObject::connect(&view, &NotificationView::LinkActivated, activated);

//////	layout.Init(nsys);

//////	layout.SetParent(nullptr);
//////	//layout.SetParent(&mainWindow);
//////	//layout.SetCorner(Qt::TopRightCorner);
//////	//layout.SetExpirationTimeouts(600ms, 400ms, 200ms);

//////	view.Init(nsys);
//////	view.SetFilterMode(view.FilterByText | view.FilterByLevel);
//////	view.show();
//////	//view.showMaximized();

//////	nsys.AddInfo("Title1", "Text1");
//////	nsys.AddInfo("Title2", "<a href = \"setings:://tralala\">Text2</a>");
//////	nsys.AddError("Title4", QtTools::ToQString(errmsg));
//////	nsys.AddWarning("Title5", QtTools::ToQString(errmsg));
//////	nsys.AddInfo("Title6", QtTools::ToQString(errmsg));
//////	nsys.AddInfo("Title7", QtTools::ToQString(errmsg));
//////	nsys.AddInfo(longTitle, QtTools::ToQString(errmsg));

//////	auto nf = nsys.CreateNotification();
//////	nf->Title("Custom");
//////	nf->Text("Some Text");
//////	nf->setProperty("backgroundColor", QColor("red"));
//////	nf->setProperty("expirationTimeout", 0);
//////	nf->ActivationLink("setings:://tralala");

//////	nsys.AddNotification(std::move(nf));

////	auto res = qapp.exec();
////	qapp.closeAllWindows();
////	return res;
////}



//#include <ext/netlib/socket_rest_supervisor.hpp>
//#include <ext/netlib/http_parser.hpp>
//#include <ext/netlib/listener.hpp>

//#include <ext/library_logger/logger.hpp>

//#include <fmt/core.h>
//#include <fmt/ostream.h>
//#include <thread>

//#include <csignal>

//class test_source : public virtual ext::netlib::socket_rest_supervisor
//{
//public:
//	ext::future<std::string> test_request();

//public:
//	test_source() = default;
//	~test_source() = default;
//};


//class test_request : public test_source::request<std::string>
//{
//	void request(ext::netlib::socket_streambuf & socket)
//	{
//		static unsigned count = 0;

//		std::ostream os(&socket);

//		char buffer[1024];
//		std::fill_n(buffer, 1024, '1');

//		os << "POST /" << count++ << " HTTP/1.1\r\n"
//		   << "Connection: keep-alive\r\n"
//		   << "Content-Length: 1024\r\n"
//		   << "Host: " << host() << "\r\n"
//		   << "\r\n";

//		os.write(buffer, 1024);
//	}

//	void response(ext::netlib::socket_streambuf & streambuf)
//	{
//		std::string name, body;
//		ext::netlib::http_parser parser(ext::netlib::http_parser::response);
//		parser.parse_status(streambuf, body);
//		parser.parse_trailing(streambuf);

//		set_value(body);
//	}
//};

//ext::future<std::string> test_source::test_request()
//{
//	return this->add_request(ext::make_intrusive<::test_request>());
//}



//int list_thread()
//{
//	using namespace std;

//	ext::netlib::listener list;
//	list.bind(8080);
//	list.listen();

//	ext::netlib::socket_streambuf recv = list.accept();
//	recv.timeout(std::chrono::steady_clock::duration::max());
//	recv.throw_errors(true);
//	recv.self_tie(false);

//	std::string method, uri, body;

//	for (;;)
//	{
//		auto res = recv.sgetc();
//		if (res == EOF) break;

//		ext::netlib::parse_http_request(recv, method, uri, body);
//		fmt::print("Got request: {} {}\n", method, uri);

//		std::ostream os(&recv);
//		os << "HTTP/1.1 200 OK\r\n"
//		   << "Content-Length: 12\r\n"
//		   << "Content-Type: text/plain\r\n"
//		   << "\r\n"
//		   << "Hello world!";

//		os.flush();
//	}

//	fmt::print("listener thread finished\n");

//	return 0;
//}


//int main()
//{
//	using namespace std;
//	std::signal(SIGPIPE, SIG_IGN);

//	ext::netlib::socket_stream_init();
//	ext::init_future_library(std::thread::hardware_concurrency());

//	std::thread thr(list_thread);


//	test_source source;
//	source.set_request_slots(99999);
//	source.set_address("localhost", "8080");

//	ext::library_logger::stream_logger lg(std::cout);
//	source.set_logger(&lg);


//	source.connect();

//	ext::future<std::string> resf;
//	for (unsigned u = 0; u < 99999; ++u)
//	{
//		resf = source.test_request();
//	}

//	auto res = resf.get();

//	fmt::print("res is {}\n", res);

//	source.disconnect().get();
//	thr.join();
//	return 0;
//}


#include <viewed/sftableset_model_qtbase.hpp>

struct test_entity
{
	std::string name;
	int volume;
};

struct traits
{
	using value_type = test_entity;
	using key_type = std::string;
	using key_eq_type = std::equal_to<>;
	using key_hash_type = std::hash<key_type>;

	using sort_pred_type = viewed::null_sorter;
	using filter_pred_type = viewed::null_filter;

	inline static const key_type & extract_key(const test_entity & val) noexcept { return val.name; }
	inline static void update(value_type & val, value_type && newval) { val = std::move(newval); }
	inline static void update(value_type & val, const value_type & newval) { val = newval; }
};

class test_model : public QAbstractTableModel, public viewed::sftableset_model_qtbase<traits>
{
	using model_base = QAbstractTableModel;
	using view_base = viewed::sftableset_model_qtbase<traits>;

public:
	virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override { return view_base::m_store.size(); }
	virtual int columnCount(const QModelIndex & parent = QModelIndex()) const override { return 2; }

	virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override { return {}; }
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override { return {}; }
};


int main(int argc, char ** argv)
{
	std::vector<test_entity> data =
	{
	    {"first", 1},
	    {"second", 2},
	    {"opla", 3},
	    {"123", 4},
	};


	test_model model;
	model.assign(std::move(data));


}

