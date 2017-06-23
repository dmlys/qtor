#include <qtor/abstract_data_source.hpp>
#include <ext/netlib/abstract_connection_controller.hpp>
#include <ext/netlib/abstract_subscription_controller.hpp>

namespace qtor
{
	class testing_data_source : 
		public abstract_data_source,
		public ext::netlib::abstract_connection_controller
	{
	protected:
		class subscription : public ext::netlib::abstract_subscription_controller
		{
			friend testing_data_source;
			testing_data_source * m_owner;
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
		std::string m_name = ":/testing-data-source-data.txt";
		torrent_list m_torrents;
		std::vector<subscription_ptr> m_subs;

	protected:
		void read_torrents(torrent_list & torrents);
		void emit_subs();

	protected:
		void do_connect_request(unique_lock lk) override;
		void do_disconnect_request(unique_lock lk) override;

	public:
		auto subscribe_torrents(torrent_handler handler) -> ext::netlib::subscription_handle override;
		auto torrent_get(torrent_id_list ids) -> ext::future<torrent_list> override;

		void set_address(std::string addr) override;
		void set_timeout(std::chrono::steady_clock::duration timeout) override {}
		void set_logger(ext::library_logger::logger * logger) override {}

	public:
		testing_data_source();
		~testing_data_source();
	};
}
