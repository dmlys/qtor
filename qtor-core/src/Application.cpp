#include <qtor/TorrentModel.hpp>
#include <qtor/Application.hqt>

namespace qtor
{
	auto Application::accquire_torrent_model() -> abstract_torrent_model_ptr
	{
		return std::make_shared<TorrentModel>(m_torrent_store);
	}

	void Application::connect()
	{
		if (not m_source) 
		{
			m_source = create_source();
		}
		
		m_source->connect();
	}

	void Application::disconnect()
	{
		m_source->disconnect();
	}
}
