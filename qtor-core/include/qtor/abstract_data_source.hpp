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

namespace qtor
{
	class abstract_data_source : public virtual ext::netlib::connection_controller
	{
	public:
		typedef std::function<void (torrent_list & list)> torrent_handler;

	public:
		virtual auto subscribe_torrents(torrent_handler handler) -> ext::netlib::subscription_handle = 0;

		virtual auto torrent_get() -> ext::future<torrent_list> = 0;
		virtual auto torrent_get(torrent_id_list ids) -> ext::future<torrent_list> = 0;

		virtual void set_address(std::string addr) = 0;
		virtual void set_timeout(std::chrono::steady_clock::duration timeout) = 0;
		virtual void set_logger(ext::library_logger::logger * logger) = 0;
		virtual void set_gui_queue(QtTools::GuiQueue * queue) = 0;

	public:
		virtual ~abstract_data_source() = default;
	};
}

