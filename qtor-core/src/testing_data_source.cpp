#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtCore/QTextCodec>
#include <QtTools/ToolsBase.hpp>

#include <chrono>

#include <ext/itoa.hpp>
#include <ext/range/line_reader.hpp>
#include <qtor/testing_data_source.hpp>

namespace qtor
{
	void testing_data_source::subscription::do_close_request(unique_lock lk)
	{
		notify_closed(std::move(lk));
	}

	void testing_data_source::subscription::do_pause_request(unique_lock lk)
	{
		notify_paused(std::move(lk));
	}

	void testing_data_source::subscription::do_resume_request(unique_lock lk)
	{
		notify_resumed(std::move(lk));
	}

	void testing_data_source::subscription::emit_data()
	{
		if (get_state() != opened) return;

		m_handler(m_owner->m_torrents);
	}

	void testing_data_source::read_torrents(torrent_list & torrents)
	{
		QFile file;
		file.setFileName(ToQString(m_name));
		file.open(QFile::ReadOnly);

		QTextStream stream(&file);
		stream.setCodec("utf-8");

		unsigned n = 0;
		ext::itoa_buffer<unsigned> buffer;

		while (not stream.atEnd())
		{
			torrents.emplace_back();
			auto & t = torrents.back();

			t.id = ext::itoa(n++, buffer);

			auto qstr = stream.readLine();
			FromQString(qstr, t.name);
		}
	}

	void testing_data_source::emit_subs()
	{
		for (auto & sub : m_subs)
			sub->emit_data();
	}

	void testing_data_source::do_connect_request(unique_lock lk)
	{
		read_torrents(m_torrents);
		
		auto * timer = static_cast<QTimer *>(m_timer);
		timer->start();

		notify_connected(std::move(lk));
	}

	void testing_data_source::do_disconnect_request(unique_lock lk)
	{
		auto * timer = static_cast<QTimer *>(m_timer);
		timer->stop();

		m_torrents.clear();
		notify_disconnected(std::move(lk));
	}

	void testing_data_source::set_address(std::string addr)
	{
		m_name = std::move(addr);
	}

	auto testing_data_source::subscribe_torrents(torrent_handler handler)
		-> ext::netlib::subscription_handle
	{
		auto sub = ext::make_intrusive<subscription>();
		sub->m_owner = this;
		sub->m_handler = std::move(handler);

		m_subs.push_back(sub);
		return sub;
	}

	auto testing_data_source::torrent_get(torrent_id_list ids)
		-> ext::future<torrent_list>
	{
		return ext::make_ready_future(m_torrents);
	}

	testing_data_source::testing_data_source()
	{
		using namespace std;
		auto * timer = new QTimer;
		m_timer = timer;

		std::chrono::milliseconds time = 3s;
		timer->setInterval(time.count());

		QObject::connect(timer, &QTimer::timeout, [this] { emit_subs(); });
	}

	testing_data_source::~testing_data_source()
	{
		delete static_cast<QTimer *>(m_timer);
	}
}
