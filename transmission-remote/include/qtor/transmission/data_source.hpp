﻿#pragma once
#include <qtor/abstract_data_source.hpp>
#include <ext/net/socket_rest_supervisor.hpp>

#include <QtTools/gui_executor.hqt>

namespace qtor {
namespace transmission
{
	class data_source :
		public abstract_data_source,
		public ext::net::socket_rest_supervisor
	{
	private:
		typedef ext::net::socket_rest_supervisor base_type;

	private:
		std::string m_encoded_uri;
		std::string m_xtransmission_session;		
		
		QtTools::gui_executor * m_executor = nullptr;

	protected:
		class request_base;
		class subscription_base;

		template <class type>
		using request = base_type::request<type, request_base>;

		class torrent_subscription;
		class torrent_request;
		class torrent_file_list_request;
		class tracker_list_request;

		class torrent_action_request;

	protected:
		void emit_signal(event_sig & sig, event_type ev) override;

	public:
		void set_address(std::string addr) override;
		void set_timeout(std::chrono::steady_clock::duration timeout) override;
		void set_logger(ext::library_logger::logger * logger) override;
		void set_gui_executor(QtTools::gui_executor * executor) override;
		auto get_gui_executor() const -> QtTools::gui_executor * override;

	public:
		virtual auto subscribe_session_stats(session_stat_handler handler) -> ext::net::subscription_handle override { return {}; }
		virtual ext::future<session_stat> get_session_stats() override { return {}; }

	public:
		auto subscribe_torrents(torrent_handler handler) -> ext::net::subscription_handle override;

		virtual ext::future<torrent_list> get_torrents() override;
		virtual ext::future<torrent_list> get_torrents(torrent_id_list ids) override;

		virtual ext::future<void> start_all_torrents() override { return ext::make_ready_future(); }
		virtual ext::future<void> stop_all_torrents() override { return ext::make_ready_future(); }

		virtual ext::future<void> start_torrents(torrent_id_list ids) override;
		virtual ext::future<void> start_torrents_now(torrent_id_list ids) override;
		virtual ext::future<void> stop_torrents(torrent_id_list ids) override;

		virtual ext::future<void> verify_torrents(torrent_id_list ids) override { return ext::make_ready_future(); }
		virtual ext::future<void> announce_torrents(torrent_id_list ids) override { return ext::make_ready_future(); }
		virtual ext::future<void> set_torrent_location(torrent_id_type id, std::string newloc, bool move) override { return ext::make_ready_future(); }

		virtual ext::future<void> remove_torrents(torrent_id_list ids) override { return ext::make_ready_future(); }
		virtual ext::future<void> purge_torrents(torrent_id_list ids) override { return ext::make_ready_future(); }

	public:
		virtual ext::future<torrent_file_list> get_torrent_files(torrent_id_type id) override;
		virtual ext::future<torrent_peer_list> get_torrent_peers(torrent_id_type id) override { return ext::make_ready_future<torrent_peer_list>({}); }

		virtual ext::future<tracker_list> get_trackers(torrent_id_type id) override;

	public:
		virtual std::string last_errormsg() const override { return base_type::last_errormsg(); }

	public:
		data_source() = default;
		~data_source() = default;
	};
}}
