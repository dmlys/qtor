#include <qtor/sqlite.hpp>
#include <qtor/sqlite-conv-qtor.hpp>
#include <qtor/utils.hpp>


namespace qtor::sqlite
{
	const std::string torrents_table_name = "torrents";
	const torrent_meta & torrents_meta()
	{
		static const qtor::torrent_meta meta;
		return meta;
	}

	const column_info & torrents_column_info()
	{
		static const column_info info = []
		{
			column_info info;

			torrent::index_type first = torrent::FirstField;
			torrent::index_type last = torrent::LastField;
			info.resize(last - first);

			for (; first != last; ++first)
			{
				auto & val = info[first];
				auto name = torrents_meta().item_name(first);
				auto type = torrents_meta().item_type(first);
				val = std::make_tuple(FromQString(name), type);
			}

			return info;
		}();

		return info;
	};


	static const char * get_type(unsigned type)
	{
		switch (type)
		{
			case sparse_container_meta::Speed:
			case sparse_container_meta::Size:
			case sparse_container_meta::Uint64: return "INT";
			case sparse_container_meta::Double: return "REAL";
			case sparse_container_meta::String: return "TEXT";

			case sparse_container_meta::DateTime:
			case sparse_container_meta::Duration:
			case sparse_container_meta::Unknown:
			default:
				return "TEXT";
		}
	}

	void create_table(sqlite3yaw::session & ses, const std::string & name, const column_info & columns)
	{
		std::string cmd;
		cmd.resize(1024);

		auto out = std::back_inserter(cmd);
		cmd = "create table ";
		out = sqlite3yaw::copy_sql_name(name, out);
		cmd += "( ";

		for (auto & col : columns)
		{
			auto & name = std::get<0>(col);
			auto & type = std::get<1>(col);
			out = sqlite3yaw::copy_sql_name(name, out);
			
			cmd += ' ';
			cmd += get_type(type);
			cmd += ',';
		}

		cmd.pop_back();
		cmd += ") ";

		ses.exec(cmd);
	}

	void create_torrents_table(sqlite3yaw::session & ses)
	{
		return create_table(ses, torrents_table_name, torrents_column_info());
	}

	void save_torrents(sqlite3yaw::session & ses, const torrent_list & torrents)
	{
		qtor::torrent_meta meta;
		auto tmeta = sqlite3yaw::load_table_meta(ses, torrents_table_name);
		auto & info  = torrents_column_info();

		auto names = info | ext::firsts;
		auto batch_range = make_batch_range(meta, names, torrents);

		sqlite3yaw::batch_insert(batch_range, ses, tmeta);
	}

	auto load_torrents(sqlite3yaw::session & ses) -> torrent_list
	{
		torrent_list result;
		qtor::torrent_meta meta;
		auto & info = torrents_column_info();

		auto names = info | ext::firsts;
		auto cmd = sqlite3yaw::select_command(torrents_table_name, names);

		return result;
	}
}
