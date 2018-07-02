#include <qtor/transmission/data_source.hpp>
#include <qtor/transmission/requests.hpp>

#include <ext/netlib/parse_url.hpp>
#include <ext/netlib/http_parser.hpp>
#include <fmt/format.h>

namespace qtor {
namespace transmission
{
	void data_source::emit_signal(event_sig & sig, event_type ev)
	{
		if (not m_queue)
			sig(ev);
		else
			m_queue->Add([&sig, ev] { sig(ev); });
	}

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

	void data_source::set_gui_queue(QtTools::GuiQueue * queue)
	{
		m_queue = queue;
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

	protected:
		template <class Data, class Handler>
		void emit_data(Data data, const Handler & handler);
		
	public:
		void request(ext::socket_stream & stream) override;
		void response(ext::socket_stream & stream) override;
		auto next_invoke() -> std::chrono::steady_clock::time_point override { return m_next; }

	public:
		virtual auto request_command() -> std::string = 0;
		virtual void parse_response(std::string body) = 0;
	};

	template <class Data, class Handler>
	void data_source::subscription_base::emit_data(Data data, const Handler & handler)
	{
		auto owner = static_cast<data_source *>(m_owner);
		auto * queue = owner->m_queue;
		if (not queue)
			handler(data);
		else
		{
			auto action = [that = ext::intrusive_ptr<subscription_base>(this), data = std::move(data), &handler]() mutable
			{
				handler(data);
			};
			
			queue->Add(action);
		}
	}

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
		ext::netlib::http_parser parser(ext::netlib::http_parser::response);
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
		ext::netlib::http_parser parser(ext::netlib::http_parser::response);
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

	class data_source::torrent_request : public request<torrent_list, request_base>
	{
		typedef data_source::request<torrent_list, request_base> base_type;

	public:
		torrent_id_list m_request_idx;

	public:
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
			auto owner = static_cast<data_source *>(m_owner);
			auto tlist = parse_torrent_list(body);
			emit_data(std::move(tlist), m_handler);
		}
	};

	auto data_source::get_torrents() -> ext::future<torrent_list>
	{
		return get_torrents({});
	}

	auto data_source::get_torrents(torrent_id_list idx) -> ext::future<torrent_list>
	{
		auto obj = ext::make_intrusive<torrent_request>();
		obj->m_request_idx = std::move(idx);
		return this->add_request(obj);
	}

	auto data_source::subscribe_torrents(torrent_handler handler) -> ext::netlib::subscription_handle
	{
		auto obj = ext::make_intrusive<torrent_subscription>();
		obj->m_handler = std::move(handler);
		return this->add_subscription(obj);
	}
}}
