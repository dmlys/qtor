#include <qtor/TorrentsModel.hpp>
#include <qtor/Application.hqt>

namespace qtor
{
	void Application::OnSourceEvent(abstract_data_source::event_type ev)
	{
		switch (ev)
		{
			case abstract_data_source::connected:        return Connected();
			case abstract_data_source::disconnected:     return Disconnected();
			case abstract_data_source::connection_error: return ConnectionError();
			case abstract_data_source::connection_lost:  return ConnectionLost();

			default: EXT_UNREACHABLE();
		}
	}

	void Application::RefreshTorrents(torrent_id_list ids)
	{
		using std::placeholders::_1;
		auto result = m_source->get_torrents(ids);
		m_executor->submit(std::move(result), std::bind(&Application::OnTorrentsRefreshed, this, _1, std::move(ids)));
	}

	void Application::OnTorrentsRefreshed(ext::future<torrent_list> result, torrent_id_list ids)
	{
		assert(result.is_ready());

		if (result.is_cancelled())
			return;

		if (result.has_value())
		{
			m_torrent_store->upsert_records(result.get());
			std::cout << "refreshed" << std::endl;
			return;
		}

		QString title = tr("Communication error");
		QString message = tr("Failed to remove torrents: %1");
		QString err;

		try
		{
			result.get();
		}
		catch (std::runtime_error & ex)
		{
			err = QtTools::ToQString(ex.what());
		}
		catch (...)
		{
			err = tr("Unknown error");
		}

		m_notificationCenter->AddError(title, message.arg(err));
	}

	void Application::OnConnectionError()
	{
		auto title = tr("Network error");
		auto body  = tr("network error: %1");

		body = body.arg(ToQString(m_source->last_errormsg()));
		m_notificationCenter->AddError(title, body);
	}

	auto Application::AccquireTorrentModel() -> abstract_torrent_model_ptr
	{
		assert(m_source);
		return std::make_shared<TorrentsModel>(m_torrent_store);
	}

	auto Application::GetSource() -> abstract_data_source_ptr
	{
		if (not m_source)
			Init();

		return m_source;
	}

	void Application::Init()
	{
		m_source = CreateSource();
		m_source->on_event([this](auto ev) { OnSourceEvent(ev); });
		m_source->set_gui_executor(m_executor);
		m_torrent_store = std::make_shared<torrent_store>(m_source);

		connect(this, &Application::ConnectionError, this, &Application::OnConnectionError);
	}

	auto Application::GetStore() -> torrent_store_ptr
	{
		if (not m_torrent_store)
			Init();

		return m_torrent_store;
	}

	void Application::OnTorrentsStarted(ext::future<void> result, torrent_id_list ids)
	{
		assert(result.is_ready());
		auto title   = tr("Communication error");
		auto message = tr("Failed to start torrents: %1");

		OnExecutionResult(result, ids, title, message);
		//RefreshTorrents(std::move(ids));
	}

	void Application::OnTorrentsStopped(ext::future<void> result, torrent_id_list ids)
	{
		assert(result.is_ready());
		auto title   = tr("Communication error");
		auto message = tr("Failed to stop torrents: %1");

		OnExecutionResult(result, ids, title, message);
		//m_torrent_store->modify(ids.begin(), ids.end(), [](auto & tor) { tor.status(qtor::torrent_status::stopped); });
		//RefreshTorrents(std::move(ids));
	}

	void Application::OnTorrentsAnnounced(ext::future<void> result, torrent_id_list ids)
	{
		assert(result.is_ready());
		auto title = tr("Communication error");
		auto message = tr("Failed to announce torrents: %1");

		OnExecutionResult(result, ids, title, message);
		//RefreshTorrents(std::move(ids));
	}

	void Application::OnTorrentsRemoved(ext::future<void> result, torrent_id_list ids)
	{
		assert(result.is_ready());
		auto title = tr("Communication error");
		auto message = tr("Failed to remove torrents: %1");

		OnExecutionResult(result, ids, title, message);
		m_torrent_store->erase(ids.begin(), ids.end());
	}

	void Application::Connect()
	{
		assert(m_source);
		m_source->connect();
	}

	void Application::Disconnect()
	{
		assert(m_source);
		m_source->disconnect();
	}

	void Application::StartTorrents(torrent_id_list ids)
	{
		assert(m_source);
		ext::future<void> result = m_source->start_torrents(ids);

		using std::placeholders::_1;
		m_executor->submit(std::move(result), std::bind(&Application::OnTorrentsStarted, this, _1, std::move(ids)));
	}

	void Application::StartNowTorrents(torrent_id_list ids)
	{
		assert(m_source);
		ext::future<void> result = m_source->start_torrents_now(ids);

		using std::placeholders::_1;
		m_executor->submit(std::move(result), std::bind(&Application::OnTorrentsStarted, this, _1, std::move(ids)));
	}

	void Application::StopTorrents(torrent_id_list ids)
	{
		assert(m_source);
		ext::future<void> result = m_source->stop_torrents(ids);

		using std::placeholders::_1;
		m_executor->submit(std::move(result), std::bind(&Application::OnTorrentsStopped, this, _1, std::move(ids)));
	}

	void Application::AnnounceTorrents(torrent_id_list ids)
	{
		assert(m_source);
		ext::future<void> result = m_source->announce_torrents(ids);

		using std::placeholders::_1;
		m_executor->submit(std::move(result), std::bind(&Application::OnTorrentsAnnounced, this, _1, std::move(ids)));
	}

	void Application::RemoveTorrents(torrent_id_list ids)
	{
		assert(m_source);
		ext::future<void> result = m_source->remove_torrents(ids);

		using std::placeholders::_1;
		m_executor->submit(std::move(result), std::bind(&Application::OnTorrentsAnnounced, this, _1, std::move(ids)));
	}

	void Application::PurgeTorrents(torrent_id_list ids)
	{
		assert(m_source);
		ext::future<void> result = m_source->purge_torrents(ids);

		using std::placeholders::_1;
		m_executor->submit(std::move(result), std::bind(&Application::OnTorrentsRemoved, this, _1, std::move(ids)));
	}

}
