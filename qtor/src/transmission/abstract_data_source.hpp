#pragma once
#include <vector>
#include <string>
#include <chrono>
#include <functional>
#include <ext/future.hpp>

#include <ext/netlib/connection_controller.hpp>
#include <ext/netlib/subscription_handle.hpp>

#include <torrent.hpp>

namespace qtor
{
	class abstract_data_source : public virtual ext::netlib::connection_controller
	{
	public:
		typedef std::function<void (torrent_list & list)> torrent_handler;

	public:
		virtual auto subscribe_torrents(torrent_handler handler) -> ext::netlib::subscription_handle = 0;
		virtual auto torrent_get(torrent_index_list ids) -> ext::future<torrent_list> = 0;

		virtual void set_address(std::string addr) = 0;
		virtual void set_timeout(std::chrono::steady_clock::duration timeout) = 0;
	};
}

