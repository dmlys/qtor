#include <array>
#include <memory>
#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <regex>

#include <ext/is_string.hpp>
#include <ext/utility.hpp>
#include <ext/base64.hpp>
#include <ext/itoa.hpp>

#include <fmt/format.h>

#include <ext/library_logger/logger.hpp>
#include <ext/library_logger/logging_macros.hpp>

#include <qtor/abstract_data_source.hpp>
#include <qtor/testing_data_source.hpp>
#include <qtor/transmission/data_source.hpp>

#include <qtor/torrent_store.hpp>
#include <qtor/TorrentModel.hpp>

#include <QtWidgets/QApplication>
#include <QtTools/GuiQueue.hqt>

#include <QtWidgets/QTableView>

#ifdef NDEBUG
#pragma  comment(lib, "libfmt-mt.lib")
#pragma  comment(lib, "openssl-crypto-mt.lib")
#pragma  comment(lib, "openssl-ssl-mt.lib")
#else
#pragma  comment(lib, "libfmt-mt-gd.lib")
#pragma  comment(lib, "openssl-crypto-mt-gd.lib")
#pragma  comment(lib, "openssl-ssl-mt-gd.lib")
#endif

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


int main(int argc, char * argv[])
{
	using namespace std;

	Q_INIT_RESOURCE(qtor_core_resource);

	QApplication app {argc, argv};

	auto source = std::make_shared<qtor::testing_data_source>();
	source->connect().get();

	auto store = std::make_shared<qtor::torrent_store>(source);
	qtor::AbstractTorrentModel * model = new qtor::TorrentModel(store);

	QTableView tableView;
	tableView.setModel(model);
	tableView.show();
	
	return app.exec();
}
