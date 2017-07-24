#include <qtor/TorrentModel.hpp>
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

	auto Application::AccquireTorrentModel() -> abstract_torrent_model_ptr
	{
		assert(m_source);
		return std::make_shared<TorrentModel>(m_torrent_store);
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
	}
}
