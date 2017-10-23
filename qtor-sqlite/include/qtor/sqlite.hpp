#pragma once
#include <tuple>
#include <vector>

#include <sqlite3yaw.hpp>
#include <sqlite3yaw_ext.hpp>
#include <qtor/torrent.hpp>

namespace qtor::sqlite
{
	using column_info = std::vector<std::tuple<sparse_container::index_type, unsigned, std::string>>;

	extern const std::string torrents_table_name;
	extern const torrent_meta & torrents_meta();
	extern const column_info & torrents_column_info();

	void create_table(sqlite3yaw::session & ses, const std::string & name, const column_info & columns);
	void create_torrents_table(sqlite3yaw::session & ses);
	void drop_torrents_table(sqlite3yaw::session & ses);

	void save_torrents(sqlite3yaw::session & ses, const torrent_list & torrents);
	auto load_torrents(sqlite3yaw::session & ses) -> torrent_list;
}
