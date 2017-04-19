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

#include <ext/netlib/codecs/url_encoding.hpp>
#include <ext/netlib/socket_stream.hpp>
#include <ext/netlib/http_response_parser.hpp>
#include <ext/netlib/http_response_stream.hpp>

#include <fmt/format.h>

#include <ext/netlib/socket_rest_supervisor.hpp>
#include <ext/library_logger/logger.hpp>
#include <ext/library_logger/logging_macros.hpp>

#include <transmission/data_source.hpp>

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

template <class Sink, class HeaderString, class ValueString>
void write_http_header(Sink & sink, const HeaderString & name, const ValueString & value)
{
	using ext::netlib::write_string;
	using ext::netlib::encode_url;
	
	write_string(sink, name);
	write_string(sink, ": ");
	encode_url(value, sink);
	write_string(sink, "\r\n");
}

template <class Sink, class Range>
std::enable_if_t<ext::is_string_range<Range>::value>
write_http_headers(Sink & sink, const Range & headers)
{
	using std::begin; using std::end;
	auto first = begin(headers);
	auto last = end(headers);

	for (;;)
	{
		if (first == last) return;
		auto && name = *first;
		++first;

		if (first == last) return;
		auto && val = *first;
		++first;

		write_http_header(
			sink, 
			std::forward<decltype(name)>(name),
			std::forward<decltype(val)>(val)
		);
	}
}

template <class Sink, class Range>
std::enable_if_t<not ext::is_string_range<Range>::value>
write_http_headers(Sink & sink, const Range & headers_map)
{
	for (auto && entity : headers_map)
	{
		using std::get;
		auto && name = get<0>(entity);
		auto && val = get<1>(entity);

		write_http_header(
			sink,
			std::forward<decltype(name)>(name),
			std::forward<decltype(val)>(val)
		);
	}
}

int main()
{
	using namespace std;
	ext::socket_stream_init();

	auto * ds = new qtor::transmission::data_source;
	ext::library_logger::stream_logger lg(clog);

	ds->set_logger(&lg);
	ds->set_timeout(10s);
	ds->set_address("https://melkiy:9091/transmission/rpc");
	//ds->set_address("http://httpbin.org/get");

	ds->connect().wait();
	auto ftorrents = ds->torrent_get({});

	try
	{
		auto list = ftorrents.get();
		for (auto & t : list)
		{
			cout << fmt::format("{} - {}", t.id, t.name) << endl;
		}
	}
	catch (std::exception & ex)
	{
		cerr << ex.what() << endl;
	}

	system("pause");
	return 0;
}
