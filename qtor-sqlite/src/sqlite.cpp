#include <qtor/sqlite.hpp>
#include <qtor/utils.hpp>

namespace qtor::sqlite
{
	const std::string torrents_table_name = "torrents";

	auto torrents_column_info() -> column_info
	{
		column_info info;
		qtor::torrent_meta meta;

		torrent::index_type first = torrent::FirstField;
		torrent::index_type last = torrent::LastField;
		info.resize(last - first);

		for (; first != last; ++first)
		{
			auto & val = info[first];
			auto name = meta.item_name(first);
			auto type = meta.item_type(first);
			val = std::make_tuple(FromQString(name), type);
		}

		return info;
	}

	void save_torrents(sqlite3yaw::session & ses, const torrent_list & torrents)
	{
		qtor::torrent_meta meta;
		auto tmeta = sqlite3yaw::load_table_meta(ses, torrents_table_name);
		auto info  = torrents_column_info();

		auto names = info | ext::firsts;
		auto batch_range = make_batch_range(meta, names, torrents);

		sqlite3yaw::batch_insert(batch_range, ses, tmeta);
	}

	auto load_torrents(sqlite3yaw::session & ses) -> torrent_list
	{
		torrent_list result;
		qtor::torrent_meta meta;
		auto info = torrents_column_info();

		auto names = info | ext::firsts;
		auto cmd = sqlite3yaw::select_command(torrents_table_name, names);

		return result;
	}
}
