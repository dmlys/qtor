#pragma once
#include <vector>
#include <string>
#include <chrono>
#include <functional>
#include <ext/future.hpp>
#include <ext/library_logger/logger.hpp>

#include <ext/netlib/connection_controller.hpp>
#include <ext/netlib/subscription_handle.hpp>

#include <QtTools/GuiQueue.hqt>

#include <qtor/torrent.hpp>
#include <qtor/torrent_file.hpp>

namespace qtor
{
	class abstract_data_source : public virtual ext::netlib::connection_controller
	{
	public:
		using torrent_handler = std::function<void (torrent_list & list)>;
		using session_stat_handler = std::function<void (session_stat & stats)>;

	public:
		virtual auto subscribe_session_stats(session_stat_handler handler) -> ext::netlib::subscription_handle = 0;
		virtual ext::future<session_stat> get_session_stats() = 0;

	public:
		virtual auto subscribe_torrents(torrent_handler handler)->ext::netlib::subscription_handle = 0;

		virtual ext::future<torrent_list> get_torrents() = 0;
		virtual ext::future<torrent_list> get_torrents(torrent_id_list ids) = 0;

		virtual ext::future<void> start_all_torrents() = 0;
		virtual ext::future<void> stop_all_torrents() = 0;

		virtual ext::future<void> start_torrents(torrent_id_list ids) = 0;
		virtual ext::future<void> start_torrents_now(torrent_id_list ids) = 0;
		virtual ext::future<void> stop_torrents(torrent_id_list ids) = 0;

		virtual ext::future<void> verify_torrents(torrent_id_list ids) = 0;
		virtual ext::future<void> announce_torrents(torrent_id_list ids) = 0;
		virtual ext::future<void> set_torrent_location(torrent_id_type id, std::string newloc, bool move) = 0;

		virtual ext::future<void> remove_torrents(torrent_id_list ids) = 0;
		virtual ext::future<void> purge_torrents(torrent_id_list ids) = 0;

	public:
		virtual ext::future<torrent_file_list> get_torrent_files(torrent_id_type id) = 0;
		virtual ext::future<torrent_peer_list> get_torrent_peers(torrent_id_type id) = 0;
		
	public:
		virtual void set_address(std::string addr) = 0;
		virtual void set_timeout(std::chrono::steady_clock::duration timeout) = 0;
		virtual void set_logger(ext::library_logger::logger * logger) = 0;
		virtual void set_gui_queue(QtTools::GuiQueue * queue) = 0;
		virtual auto get_gui_queue() const -> QtTools::GuiQueue * = 0;

	public:
		virtual ~abstract_data_source() = default;
	};
}

