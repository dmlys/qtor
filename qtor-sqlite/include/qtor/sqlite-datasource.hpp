#pragma once
#include <ext/netlib/abstract_connection_controller.hpp>
#include <ext/netlib/abstract_subscription_controller.hpp>
#include <qtor/abstract_data_source.hpp>


namespace qtor::sqlite
{
	class sqlite_datasource :
		public abstract_data_source,
		public ext::netlib::abstract_connection_controller
	{
	protected:
		class subscription : public ext::netlib::abstract_subscription_controller
		{
			friend sqlite_datasource;
			sqlite_datasource * m_owner;
			torrent_handler m_handler;

		protected:
			void do_close_request(unique_lock lk) override;
			void do_pause_request(unique_lock lk) override;
			void do_resume_request(unique_lock lk) override;

		public:
			void emit_data();
		};

		typedef ext::intrusive_ptr<subscription> subscription_ptr;

	protected:
		void * m_timer = nullptr;
		void * m_ses = nullptr;
		std::string m_path;
		torrent_list m_torrents;
		std::vector<subscription_ptr> m_subs;

	protected:
		void emit_subs();

	protected:
		void do_connect_request(unique_lock lk) override;
		void do_disconnect_request(unique_lock lk) override;

	public:
		auto subscribe_torrents(torrent_handler handler) -> ext::netlib::subscription_handle override;
		
		virtual ext::future<torrent_list> get_torrents() override;
		virtual ext::future<torrent_list> get_torrents(torrent_id_list ids) override;

		virtual ext::future<void> start_all_torrents() override { return ext::make_ready_future(); }
		virtual ext::future<void> stop_all_torrents() override { return ext::make_ready_future(); }

		virtual ext::future<void> start_torrents(torrent_id_list ids) override { return ext::make_ready_future(); }
		virtual ext::future<void> start_now_torrents(torrent_id_list ids) override { return ext::make_ready_future(); }
		virtual ext::future<void> stop_torrents(torrent_id_list ids) override { return ext::make_ready_future(); }

		virtual ext::future<void> verify_torrents(torrent_id_list ids) override { return ext::make_ready_future(); }
		virtual ext::future<void> announce_torrents(torrent_id_list ids) override { return ext::make_ready_future(); }
		virtual ext::future<void> set_torrent_location(torrent_id_type id, std::string newloc, bool move) override { return ext::make_ready_future(); }

		virtual ext::future<void> remove_torrents(torrent_id_list ids) override { return ext::make_ready_future(); }
		virtual ext::future<void> purge_torrents(torrent_id_list ids) override { return ext::make_ready_future(); }

		void set_address(std::string addr) override;
		void set_timeout(std::chrono::steady_clock::duration timeout) override {}
		void set_logger(ext::library_logger::logger * logger) override {}
		void set_gui_queue(QtTools::GuiQueue * queue) override {}

	public:
		sqlite_datasource();
		~sqlite_datasource();
	};
}
