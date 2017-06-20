#include <qtor/transmission/data_source.hpp>
#include <qtor/transmission/requests.hpp>

#include <ext/netlib/parse_url.hpp>
#include <ext/netlib/http_response_parser.hpp>
#include <fmt/format.h>

namespace qtor {
namespace transmission
{
	void data_source::set_address(std::string addr)
	{
		auto parsed = ext::netlib::parse_url(addr);
		m_encoded_uri = parsed.path;

		std::string service = parsed.port.empty() ? "http" : parsed.port;
		base_type::set_address(parsed.host, std::move(service));
	}

	void data_source::set_timeout(std::chrono::steady_clock::duration timeout)
	{
		base_type::set_timeout(timeout);
	}

	void data_source::set_logger(ext::library_logger::logger * logger)
	{
		base_type::set_logger(logger);
	}
	
	class data_source::request_base : public base_type::request_base
	{
	public:
		void request(ext::socket_stream & stream) override;
		void response(ext::socket_stream & stream) override;

	public:
		virtual auto request_command() -> std::string = 0;
		virtual void parse_response(std::string body) = 0;
	};

	class data_source::subscription_base : public subscription
	{
		std::chrono::steady_clock::time_point m_next = std::chrono::steady_clock::now();
		std::chrono::steady_clock::duration m_delay = std::chrono::seconds(2);

	public:
		void request(ext::socket_stream & stream) override;
		void response(ext::socket_stream & stream) override;
		auto next_invoke() -> std::chrono::steady_clock::time_point override { return m_next; }

	public:
		virtual auto request_command() -> std::string = 0;
		virtual void parse_response(std::string body) = 0;
	};

	void data_source::subscription_base::request(ext::socket_stream & stream)
	{
		auto owner = static_cast<data_source *>(m_owner);
		auto & uri = owner->m_encoded_uri;
		auto & session = owner->m_xtransmission_session;

		auto body = request_command();

		stream
			<< "POST " << uri << " HTTP/1.1\r\n"
			<< "Host: " << host() << "\r\n"
			<< "Content-Length: " << body.size() << "\r\n"
			<< "Accept-Encoding: deflate, gzip\r\n";

		if (not session.empty())
			stream << "X-Transmission-Session-Id: " << session << "\r\n";

		stream << "\r\n" << body;
	}

	void data_source::subscription_base::response(ext::socket_stream & stream)
	{
		auto owner = static_cast<data_source *>(m_owner);
		auto & session = owner->m_xtransmission_session;

		std::string name, body;
		ext::netlib::http_response_parser parser;
		parser.parse_status(stream, body);

		int code = parser.http_code();
		if (code / 100 == 2)
		{
			ext::netlib::parse_http_response(parser, stream, body);
			m_next = std::chrono::steady_clock::now() + m_delay;
			parse_response(std::move(body));
		}
		else if (code == 409)
		{
			while (parser.parse_header(stream, name, body))
				if (name == "X-Transmission-Session-Id")
					session = body;

			parse_trailing(parser, stream);
		}
		else
		{
			parse_trailing(parser, stream);
			auto err = fmt::format("Bad http response: {}, {}", code, body);
			throw std::runtime_error(std::move(err));
		}
	}

	void data_source::request_base::request(ext::socket_stream & stream)
	{
		auto owner = static_cast<data_source *>(m_owner);
		auto & uri = owner->m_encoded_uri;
		auto & session = owner->m_xtransmission_session;

		auto body = request_command();

		stream
			<< "POST " << uri << " HTTP/1.1\r\n"
			<< "Host: " << host() << "\r\n"
			<< "Content-Length: " << body.size() << "\r\n"
			<< "Accept-Encoding: deflate, gzip\r\n";

		if (not session.empty())
			stream << "X-Transmission-Session-Id: " << session << "\r\n";

		stream << "\r\n" << body;
	}

	void data_source::request_base::response(ext::socket_stream & stream)
	{
		auto owner = static_cast<data_source *>(m_owner);
		auto & session = owner->m_xtransmission_session;

		std::string name, body;
		ext::netlib::http_response_parser parser;
		parser.parse_status(stream, body);

		int code = parser.http_code();
		if (code / 100 == 2)
		{
			ext::netlib::parse_http_response(parser, stream, body);
			parse_response(std::move(body));
		}
		else if (code == 409)
		{
			while (parser.parse_header(stream, name, body))
				if (name == "X-Transmission-Session-Id")
					session = body;

			parse_trailing(parser, stream);
			set_repeat();
		}
		else
		{
			parse_trailing(parser, stream);
			auto err = fmt::format("Bad http response: {}, {}", code, body);
			throw std::runtime_error(std::move(err));
		}
	}

	class data_source::post_continuation : public ext::continuation_base
	{
		data_source * m_owner;
		QtTools::GuiQueue::action_type m_action;

	public:
		post_continuation(ext::intrusive_ptr<continuation_type> cont, data_source * owner) 
			: m_owner(owner) 
		{
			m_action = [cont = std::move(cont)] { cont->continuate(); };
		}

		void continuate() noexcept override;
	};

	void data_source::post_continuation::continuate() noexcept
	{
		m_owner->m_queue.Add(std::move(m_action));
	}

	class data_source::torrent_request : public request<torrent_list, request_base>
	{
		typedef data_source::request<torrent_list, request_base> base_type;

	public:
		torrent_id_list m_request_idx;

	public:
		bool add_continuation(continuation_type * continuation) noexcept override
		{
			auto owner = static_cast<data_source *>(m_owner);
			ext::intrusive_ptr<continuation_type> ptr {continuation, ext::noaddref};

			continuation = new post_continuation(std::move(ptr), owner);
			return base_type::add_continuation(continuation);
		}

		auto request_command() -> std::string override
		{ 
			return make_request_command(constants::torrent_get, m_request_idx);
		}

		void parse_response(std::string body) override
		{
			auto tlist = parse_torrent_list(body);
			set_value(std::move(tlist));
		}
	};

	class data_source::torrent_subscription : public subscription_base
	{
	public:
		torrent_id_list m_request_idx;
		torrent_handler m_handler;

	public:
		auto request_command() -> std::string override
		{
			return make_request_command(constants::torrent_get, m_request_idx);
		}

		void parse_response(std::string body) override
		{
			auto tlist = parse_torrent_list(body);
			m_handler(tlist);
		}
	};


	auto data_source::torrent_get(torrent_id_list idx) -> ext::future<torrent_list>
	{
		auto obj = ext::make_intrusive<torrent_request>();
		obj->m_request_idx = std::move(idx);
		return this->add_request(obj);
	}

	auto data_source::subscribe_torrents(torrent_handler handler) -> ext::netlib::subscription_handle
	{
		auto obj = ext::make_intrusive<torrent_subscription>();
		obj->m_request_idx;
		obj->m_handler = std::move(handler);
		return this->add_subscription(obj);
	}
}}
