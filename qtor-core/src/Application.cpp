#include <qtor/TorrentsModel.hpp>
#include <qtor/Application.hqt>

namespace qtor
{
	void Application::OnEventSource(abstract_data_source::event_type ev)
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

	void Application::OnExecutionResult(ext::future<void> result, QString errTitle, QString errMessageTemplate)
	{
		auto handler = [this, errTitle = std::move(errTitle), errMessageTemplate = std::move(errMessageTemplate)](ext::future<void> result)
		{
			assert(result.is_ready());
			if (not result.has_exception()) return result.get();

			QString errText;

			try
			{
				result.get();
			}
			catch (std::runtime_error & ex)
			{
				errText += QtTools::ToQString(ex.what());
			}
			catch (...)
			{
				errText = tr("Unknown error");
			}

			auto notification = m_notificationCenter->CreateNotification();
			notification->Title(errTitle);
			notification->Text(errMessageTemplate.arg(errText));

			m_notificationCenter->AddNotification(std::move(notification));
		};

		m_executor->submit(std::move(result), handler);
	}

	void Application::OnConnectionError()
	{
		auto title = tr("Network error");
		auto body = tr("network error: %1");

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
		m_source->on_event([this](auto ev) { OnEventSource(ev); });
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
		ext::future<void> result = m_source->start_torrents(std::move(ids));

		auto title = tr("Communication error");
		auto message = tr("Failed to start torrents: %1");

		OnExecutionResult(std::move(result), title, message);
	}

	void Application::StartNowTorrents(torrent_id_list ids)
	{
		assert(m_source);
		ext::future<void> result = m_source->start_torrents_now(std::move(ids));

		auto title = tr("Communication error");
		auto message = tr("Failed to start torrents: %1");

		OnExecutionResult(std::move(result), title, message);
	}

	void Application::StopTorrents(torrent_id_list ids)
	{
		assert(m_source);
		ext::future<void> result = m_source->stop_torrents(std::move(ids));

		auto title = tr("Communication error");
		auto message = tr("Failed to stop torrents: %1");

		OnExecutionResult(std::move(result), title, message);
	}

	void Application::AnnounceTorrents(torrent_id_list ids)
	{
		assert(m_source);
		ext::future<void> result = m_source->announce_torrents(std::move(ids));

		auto title = tr("Communication error");
		auto message = tr("Failed to announce torrents: %1");

		OnExecutionResult(std::move(result), title, message);
	}

	void Application::RemoveTorrents(torrent_id_list ids)
	{
		assert(m_source);
		ext::future<void> result = m_source->remove_torrents(std::move(ids));

		auto title = tr("Communication error");
		auto message = tr("Failed to remove torrents: %1");

		OnExecutionResult(std::move(result), title, message);
	}

	void Application::PurgeTorrents(torrent_id_list ids)
	{
		assert(m_source);
		ext::future<void> result = m_source->purge_torrents(std::move(ids));

		auto title = tr("Communication error");
		auto message = tr("Failed to purge torrents: %1");

		OnExecutionResult(std::move(result), title, message);
	}

}
