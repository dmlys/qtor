#include <qtor/torrent_store.hpp>

namespace qtor
{
	auto torrent_store::subscribe() -> ext::netlib::subscription_handle
	{
		auto handler = [this](auto recs) { assign_records(std::move(recs)); };
		return m_source->subscribe_torrents(handler);
	}
}
