#pragma once
#include <qtor/abstract_data_source.hpp>
#include <ext/netlib/socket_rest_supervisor.hpp>

#include <QtTools/GuiQueue.hqt>

namespace qtor {
namespace transmission
{
	class data_source :
		public abstract_data_source,
		public ext::netlib::socket_rest_supervisor
	{
	private:
		typedef ext::netlib::socket_rest_supervisor base_type;

	private:
		std::string m_encoded_uri;
		std::string m_xtransmission_session;		
		
		QtTools::GuiQueue m_queue;

	protected:
		class request_base;
		class subscription_base;
		class torrent_subscription;
		class torrent_request;

	//protected:
	//	void emit_signal(event_sig & sig, event_type ev) override;

	public:
		void set_address(std::string addr) override;
		void set_timeout(std::chrono::steady_clock::duration timeout) override;
		void set_logger(ext::library_logger::logger * logger) override;

		auto subscribe_torrents(torrent_handler handler) -> ext::netlib::subscription_handle override;

		auto torrent_get() -> ext::future<torrent_list> override;
		auto torrent_get(torrent_id_list ids) -> ext::future<torrent_list> override;

	public:
		data_source() = default;
		~data_source() = default;
	};
}}
