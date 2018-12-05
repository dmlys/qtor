#include <qtor/sqlite-datasource.hpp>
#include <qtor/sqlite.hpp>
#include <qtor/sqlite-conv-qtor.hpp>

namespace qtor::sqlite
{
	void sqlite_datasource::subscription::emit_data()
	{
		if (get_state() != opened) return;

		m_handler(m_owner->m_torrents);
	}

	void sqlite_datasource::emit_subs()
	{
		for (auto & sub : m_subs)
			sub->emit_data();
	}

	void sqlite_datasource::do_connect_request(unique_lock lk)
	{
		try
		{
			auto ses = new sqlite3yaw::session(m_path);
			m_ses = ses;

			m_torrents = load_torrents(*ses);
			notify_connected(std::move(lk));

			emit_subs();
		}
		catch (sqlite3yaw::sqlite_error & ex)
		{
			std::cerr << ex.what() << std::endl;
			notify_disconnected(std::move(lk));
			return;
		}		
	}

	void sqlite_datasource::do_disconnect_request(unique_lock lk)
	{
		delete static_cast<sqlite3yaw::session *>(m_ses);
		m_ses = nullptr;

		m_torrents.clear();
		notify_disconnected(std::move(lk));
	}

	void sqlite_datasource::set_address(std::string addr)
	{
		m_path = std::move(addr);
	}

	auto sqlite_datasource::subscribe_torrents(torrent_handler handler)
		-> ext::netlib::subscription_handle
	{
		auto sub = ext::make_intrusive<subscription>();
		sub->m_owner = this;
		sub->m_handler = std::move(handler);

		m_subs.push_back(sub);
		sub->emit_data();
		return {sub};
	}

	auto sqlite_datasource::get_torrents(torrent_id_list ids)
		-> ext::future<torrent_list>
	{
		return ext::make_ready_future(m_torrents);
	}

	auto sqlite_datasource::get_torrents()
		-> ext::future<torrent_list>
	{
		return ext::make_ready_future(m_torrents);
	}

	auto sqlite_datasource::get_torrent_files(torrent_id_type id) -> ext::future<torrent_file_list>
	{
		assert(m_ses);
		auto files = load_torrent_files(*static_cast<sqlite3yaw::session *>(m_ses), id);
		return ext::make_ready_future(std::move(files));
	}

	sqlite_datasource::sqlite_datasource()
	{
		using namespace std;
	}

	sqlite_datasource::~sqlite_datasource()
	{		
		delete static_cast<sqlite3yaw::session *>(m_ses);
		m_ses = nullptr;
	}

	void sqlite_datasource::subscription::do_close_request(unique_lock lk)
	{
		notify_closed(std::move(lk));
	}

	void sqlite_datasource::subscription::do_pause_request(unique_lock lk)
	{
		notify_paused(std::move(lk));
	}

	void sqlite_datasource::subscription::do_resume_request(unique_lock lk)
	{
		notify_resumed(std::move(lk));
	}
}
