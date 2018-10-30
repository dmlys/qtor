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
	extern const std::string torrent_files_table_name;

	const model_meta & torrents_meta();
	const model_meta & torrent_files_meta();

	const column_info & torrents_column_info();
	const column_info & torrent_files_column_info();

	void create_table(sqlite3yaw::session & ses, const std::string & name, const column_info & columns);
	void create_torrents_table(sqlite3yaw::session & ses);
	void drop_torrents_table(sqlite3yaw::session & ses);
	void create_torrent_files_table(sqlite3yaw::session & ses);
	void drop_torrent_files_table(sqlite3yaw::session & ses);

	void save_torrents(sqlite3yaw::session & ses, const torrent_list & torrents);
	auto load_torrents(sqlite3yaw::session & ses) -> torrent_list;
}
