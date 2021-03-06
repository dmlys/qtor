﻿#include <qtor/transmission/data_source.hpp>
#include <qtor/transmission/requests.hpp>

#include <ext/net/parse_url.hpp>
#include <ext/net/http_parser.hpp>
#include <ext/library_logger/logging_macros.hpp>

#include <regex>
#include <fmt/format.h>

namespace qtor {
namespace transmission
{
	void data_source::emit_signal(event_sig & sig, event_type ev)
	{
		if (not m_executor)
			sig(ev);
		else
			m_executor->submit([&sig, ev] { sig(ev); });
	}

	void data_source::set_address(std::string addr)
	{
		auto parsed = ext::net::parse_url(addr);
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

	void data_source::set_gui_executor(QtTools::gui_executor * executor)
	{
		m_executor = executor;
	}

	auto data_source::get_gui_executor() const -> QtTools::gui_executor *
	{
		return m_executor;
	}
	
	class data_source::request_base : public base_type::request_base
	{
	public:
		void request(ext::net::socket_streambuf & streambuf) override;
		void response(ext::net::socket_streambuf & streambuf) override;

	public:
		virtual auto request_command() -> std::string = 0;
		virtual void parse_response(std::string body) = 0;
	};

	class data_source::subscription_base : public subscription
	{
	protected:
		std::chrono::steady_clock::time_point m_next = std::chrono::steady_clock::now();
		std::chrono::steady_clock::duration m_delay = std::chrono::seconds(2);

	protected:
		template <class Data, class Handler>
		void emit_data(Data data, const Handler & handler);
		
	public:
		void request(ext::net::socket_streambuf & streambuf) override;
		void response(ext::net::socket_streambuf & streambuf) override;
		auto next_invoke() -> std::chrono::steady_clock::time_point override { return m_next; }

	public:
		virtual auto request_command() -> std::string = 0;
		virtual void process_response(std::string body) = 0;
	};

	template <class Data, class Handler>
	void data_source::subscription_base::emit_data(Data data, const Handler & handler)
	{
		auto owner = static_cast<data_source *>(m_owner);
		auto * executor = owner->m_executor;
		if (not executor)
			handler(data);
		else
		{
			auto action = [that = ext::intrusive_ptr<subscription_base>(this), data = std::move(data), &handler]() mutable
			{
				handler(data);
			};
			
			executor->submit(action);
		}
	}

	void data_source::subscription_base::request(ext::net::socket_streambuf & streambuf)
	{
		std::ostream stream(&streambuf);

		auto owner = static_cast<data_source *>(m_owner);
		auto & uri = owner->m_encoded_uri;
		auto & session = owner->m_xtransmission_session;

		auto body = request_command();
		auto command = extract_command(body);

		EXTLL_DEBUG_FMT(logger(), "subscription {}: sending {} command", fmt::ptr(this), command);

		stream
			<< "POST " << uri << " HTTP/1.1\r\n"
			<< "Host: " << host() << "\r\n"
			<< "Content-Length: " << body.size() << "\r\n"
			<< "Accept-Encoding: deflate, gzip\r\n";

		if (not session.empty())
			stream << "X-Transmission-Session-Id: " << session << "\r\n";

		stream << "\r\n" << body;

		EXTLL_TRACE_FMT(logger(), "subscription {}: sent {} command", fmt::ptr(this), command);
	}

	void data_source::subscription_base::response(ext::net::socket_streambuf & streambuf)
	{
		EXTLL_DEBUG_FMT(logger(), "subscription {}: receiving answer", fmt::ptr(this));

		auto owner = static_cast<data_source *>(m_owner);
		auto & session = owner->m_xtransmission_session;

		std::string name, body;
		ext::net::http_parser parser(ext::net::http_parser::response);
		parser.parse_status(streambuf, body);

		int code = parser.http_code();
		if (code / 100 == 2)
		{
			ext::net::parse_http_response(parser, streambuf, body);
			m_next = std::chrono::steady_clock::now() + m_delay;
			process_response(std::move(body));
		}
		else if (code == 409)
		{
			while (parser.parse_header(streambuf, name, body))
				if (name == "X-Transmission-Session-Id")
					session = body;

			EXTLL_INFO_FMT(logger(), "subscription {}: got HTTP 409 code, Transmission-Session-Id - {}", fmt::ptr(this), session);

			parser.parse_trailing(streambuf);
		}
		else
		{
			parser.parse_trailing(streambuf);

			EXTLL_WARN_FMT(logger(), "subscription {}: Bad http response: {}, {}", fmt::ptr(this), code, body);
			auto err = fmt::format("Bad http response: {}, {}", code, body);
			throw std::runtime_error(std::move(err));
		}
	}

	void data_source::request_base::request(ext::net::socket_streambuf & streambuf)
	{
		std::ostream stream(&streambuf);

		auto owner = static_cast<data_source *>(m_owner);
		auto & uri = owner->m_encoded_uri;
		auto & session = owner->m_xtransmission_session;

		auto body = request_command();
		auto command = extract_command(body);

		EXTLL_DEBUG_FMT(logger(), "request {}: sending {} command", fmt::ptr(this), command);

		stream
			<< "POST " << uri << " HTTP/1.1\r\n"
			<< "Host: " << host() << "\r\n"
			<< "Content-Length: " << body.size() << "\r\n"
			<< "Accept-Encoding: deflate, gzip\r\n";

		if (not session.empty())
			stream << "X-Transmission-Session-Id: " << session << "\r\n";

		stream << "\r\n" << body;

		EXTLL_TRACE_FMT(logger(), "request {}: sent {} command", fmt::ptr(this), command);
	}

	void data_source::request_base::response(ext::net::socket_streambuf & streambuf)
	{
		EXTLL_DEBUG_FMT(logger(), "request {}: receiving answer", fmt::ptr(this));

		auto owner = static_cast<data_source *>(m_owner);
		auto & session = owner->m_xtransmission_session;

		std::string name, body;
		ext::net::http_parser parser(ext::net::http_parser::response);
		parser.parse_status(streambuf, body);

		int code = parser.http_code();
		if (code / 100 == 2)
		{
			ext::net::parse_http_response(parser, streambuf, body);
			parse_response(std::move(body));
		}
		else if (code == 409)
		{
			while (parser.parse_header(streambuf, name, body))
				if (name == "X-Transmission-Session-Id")
					session = body;

			EXTLL_INFO_FMT(logger(), "subscription {}: get HTTP 409 code, Transmission-Session-Id - {}", fmt::ptr(this), session);

			parser.parse_trailing(streambuf);
			set_repeat();
		}
		else
		{
			parser.parse_trailing(streambuf);
\
			EXTLL_WARN_FMT(logger(), "subscription {}: Bad http response: {}, {}", fmt::ptr(this), code, body);
			auto err = fmt::format("Bad http response: {}, {}", code, body);
			throw std::runtime_error(std::move(err));
		}
	}

	class data_source::torrent_request : public request<torrent_list>
	{
		typedef data_source::request<torrent_list> base_type;

	public:
		torrent_id_list m_request_idx;

	public:
		auto request_command() -> std::string override
		{ 
			return make_torrent_get_command(m_request_idx);
		}

		void parse_response(std::string body) override
		{
			auto tlist = parse_torrent_list(body);
			set_value(std::move(tlist));
		}
	};

	class data_source::torrent_file_list_request : public request<torrent_file_list>
	{
		using base_type = data_source::request<torrent_file_list>;

	public:
		torrent_id_type m_request_id;

	public:
		auto request_command() -> std::string override
		{
			auto cmd = make_torrent_files_get_command(m_request_id);
			return cmd;
		}

		void parse_response(std::string body) override
		{
			auto list = parse_torrent_file_list(body);
			set_value(std::move(list));
		}
	};

	class data_source::tracker_list_request : public request<tracker_list>
	{
		using base_type = data_source::request<tracker_list>;

	public:
		torrent_id_type m_request_id;

	public:
		auto request_command() -> std::string override
		{
			auto cmd = make_tracker_list_get_command(m_request_id);
			return cmd;
		}

		void parse_response(std::string body) override
		{
			auto list = parse_tracker_list(body);
			set_value(std::move(list));
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
			return make_torrent_get_command(m_request_idx);
		}

		void process_response(std::string body) override
		{
			auto tlist = parse_torrent_list(body);
			emit_data(std::move(tlist), m_handler);
		}
	};


	class data_source::torrent_action_request : public request<void>
	{
		using base_type = data_source::request<void>;

	public:
		torrent_id_list m_ids;
		std::string m_action;

	public:
		auto request_command() -> std::string override
		{
			return make_request_command(m_action, m_ids);
		}

		void parse_response(std::string body) override
		{
			parse_command_response(body);
			set_value();
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
		return this->add_request(std::move(obj));
	}

	auto data_source::subscribe_torrents(torrent_handler handler) -> ext::net::subscription_handle
	{
		auto obj = ext::make_intrusive<torrent_subscription>();
		obj->m_handler = std::move(handler);
		return this->add_subscription(std::move(obj));
	}

	auto data_source::get_torrent_files(torrent_id_type idx) -> ext::future<torrent_file_list>
	{
		auto obj = ext::make_intrusive<torrent_file_list_request>();
		obj->m_request_id = std::move(idx);
		return this->add_request(std::move(obj));
	}

	auto data_source::get_trackers(torrent_id_type idx) -> ext::future<tracker_list>
	{
		auto obj = ext::make_intrusive<tracker_list_request>();
		obj->m_request_id = std::move(idx);
		return this->add_request(std::move(obj));
	}

	auto data_source::start_torrents(torrent_id_list ids) -> ext::future<void>
	{
		auto obj = ext::make_intrusive<torrent_action_request>();
		obj->m_ids = std::move(ids);
		obj->m_action = torrent_start;

		return this->add_request(std::move(obj));
	}

	auto data_source::start_torrents_now(torrent_id_list ids) -> ext::future<void>
	{
		auto obj = ext::make_intrusive<torrent_action_request>();
		obj->m_ids = std::move(ids);
		obj->m_action = torrent_start_now;

		return this->add_request(std::move(obj));
	}

	auto data_source::stop_torrents(torrent_id_list ids) -> ext::future<void>
	{
		auto obj = ext::make_intrusive<torrent_action_request>();
		obj->m_ids = std::move(ids);
		obj->m_action = torrent_stop;

		return this->add_request(std::move(obj));
	}
}}
