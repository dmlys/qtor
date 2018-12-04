#pragma once
#include <tuple>
#include <vector>

#include <sqlite3yaw.hpp>
#include <sqlite3yaw_ext.hpp>

#include <qtor/torrent.hpp>
#include <qtor/torrent_file.hpp>

namespace qtor::sqlite
{
	extern const std::string torrents_table_name;
	extern const std::string torrent_files_table_name;

	void create_table(sqlite3yaw::session & ses, const std::string & name, const model_meta & meta);
	void create_torrents_table(sqlite3yaw::session & ses);
	void drop_torrents_table(sqlite3yaw::session & ses);
	void create_torrent_files_table(sqlite3yaw::session & ses);
	void drop_torrent_files_table(sqlite3yaw::session & ses);

	void save_torrents(sqlite3yaw::session & ses, const torrent_list & torrents);
	auto load_torrents(sqlite3yaw::session & ses) -> torrent_list;

	void save_torrent_files(sqlite3yaw::session & ses, const torrent_file_list & files, const torrent_id_type & id);
	auto load_torrent_files(sqlite3yaw::session & ses, const torrent_id_type & id);
}
