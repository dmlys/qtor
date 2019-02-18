#include <qtor/torrent_store.hpp>

namespace qtor
{
	auto torrent_store::subscribe() -> ext::net::subscription_handle
	{
		auto handler = [this](auto recs) { assign_records(std::move(recs)); };
		return m_source->subscribe_torrents(handler);
	}

	torrent_store::torrent_store(std::shared_ptr<abstract_data_source> source)
		: m_source(std::move(source)) 
	{

	}
}
