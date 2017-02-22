#include <array>
#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <ext/is_string.hpp>
#include <ext/utility.hpp>
#include <ext/base64.hpp>
#include <ext/netlib/codecs/url_encoding.hpp>
#include <ext/netlib/socket_stream.hpp>
#include <ext/netlib/http_response_parser.hpp>
#include <ext/netlib/http_response_stream.hpp>

#pragma  comment(lib, "openssl-mt-gd.lib")
#pragma  comment(lib, "openssl-eay-mt-gd.lib")

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

template <class Sink, class String, class Headers = decltype(empty_headers)>
void write_http_get(Sink & sink, const String & resource, const Headers & headers = empty_headers)
{
	using ext::netlib::write_string;
	write_string(sink, "GET ");
	write_string(sink, resource);
	write_string(sink, " HTTP/1.1\r\n");

	auto chops = {
		"Host", "httpbin.org",
		"Connection", "close",
		//"Accept-Encoding", "gzip",
	};

	write_http_headers(sink, chops);
	write_string(sink, "\r\n");
}


int main()
{
	using namespace std;

	ext::socket_stream_init();
	ext::socket_stream sock;

	sock.connect("httpbin.org", "http");

	auto & ss = sock;

	auto headers = {"Connection", "close", "Accept-Encoding", "gzip"};
	write_http_get(ss, "/get", headers);
	cout << ss.rdbuf() << endl;

	//write_http_headers(ss, hmap);

	//std::string response;
	//int code = ext::netlib::parse_http_response(sock, response);
	//cout << response << endl;

	system("pause");
	return 0;
}
