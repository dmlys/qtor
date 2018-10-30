#include <qtor/sqlite.hpp>
#include <qtor/sqlite-conv-qtor.hpp>
#include <qtor/utils.hpp>

#include <ext/range/adaptors/getted.hpp>


namespace qtor::sqlite
{
	const std::string torrents_table_name = "torrents";
	const std::string torrent_files_table_name = "torrent_files";

	const model_meta & torrents_meta()
	{
		static const qtor::torrent_meta meta;
		return meta;
	}

	const model_meta & torrent_files_meta()
	{
		static const qtor::torrent_file_meta meta;
		return meta;
	}

	static column_info column_info_from_meta(const model_meta & meta)
	{
		auto field_count = meta.item_count();
		column_info info(field_count);

		for (model_meta::index_type i = 0; i < field_count; ++i)
		{
			auto & val = info[i];
			auto name = meta.item_name(i);
			auto type = meta.item_type(i);

			val = std::make_tuple(i, type, FromQString(name));
		}

		return info;
	}

	const column_info & torrents_column_info()
	{
		static const column_info info = []
		{		
			auto & meta = torrents_meta();
			return column_info_from_meta(meta);
		}();

		return info;
	}

	const column_info & torrent_files_column_info()
	{
		static const column_info info = []
		{
			const model_meta & meta = torrent_files_meta();
			return column_info_from_meta(meta);
		}();

		return info;
	}


	static const char * get_type(unsigned type)
	{
		switch (type)
		{
			case model_meta::Speed:
			case model_meta::Size:
			case model_meta::Uint64: return "INT";
			case model_meta::Double: return "REAL";
			case model_meta::String: return "TEXT";

			case model_meta::DateTime:
			case model_meta::Duration:
			case model_meta::Unknown:
			default:
				return "TEXT";
		}
	}

	void create_table(sqlite3yaw::session & ses, const std::string & name, const column_info & columns)
	{
		std::string cmd;
		cmd.resize(1024);

		cmd = "create table if not exists ";
		sqlite3yaw::escape_sql_name(name, cmd);
		cmd += "( ";

		for (auto & col : columns)
		{
			auto & type = std::get<1>(col);
			auto & name = std::get<2>(col);
			sqlite3yaw::escape_sql_name(name, cmd);
			
			cmd += ' ';
			cmd += get_type(type);
			if (name == "Id")
				cmd += " PRIMARY KEY";

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

	void drop_torrents_table(sqlite3yaw::session & ses)
	{
		std::string cmd = "drop table ";
		sqlite3yaw::escape_sql_name(torrents_table_name, cmd);

		ses.exec_ex(cmd);
	}

	void create_torrent_files_table(sqlite3yaw::session & ses)
	{
		return create_table(ses, torrent_files_table_name, torrent_files_column_info());
	}

	void drop_torrent_files_table(sqlite3yaw::session & ses)
	{
		std::string cmd = "drop table ";
		sqlite3yaw::escape_sql_name(torrent_files_table_name);

		ses.exec_ex(cmd);
	}


	void save_torrents(sqlite3yaw::session & ses, const torrent_list & torrents)
	{
		qtor::torrent_meta meta;
		auto tmeta = sqlite3yaw::load_table_meta(ses, torrents_table_name);
		auto & info  = torrents_column_info();

		auto names = info | ext::getted<2>;
		auto batch_range = make_batch_range(meta, names, torrents);
		sqlite3yaw::batch_upsert(batch_range, ses, tmeta);

	}

	/*
	struct conv_type
	{
		unsigned idx = 0;
		sqlite3yaw::statement * stmt;
		torrent * torr;

#define OPERATOR(Type)                                              \
		void operator()(torrent & (torrent::*pmf)(Type val))        \
		{                                                           \
			Type val;                                               \
			sqlite3yaw::get(*stmt, idx++, val);                     \
			(torr->*pmf)(std::move(val));                           \
		}                                                           \

		OPERATOR(string_type);
		OPERATOR(uint64_type);
		OPERATOR(double);
		OPERATOR(bool);
		OPERATOR(datetime_type);
		OPERATOR(duration_type);

#undef OPERATOR

		conv_type(sqlite3yaw::statement & stmt, torrent & torr)
			: stmt(&stmt), torr(&torr) {}
	};
	*/

	static torrent load_torrent(sqlite3yaw::statement & stmt, const column_info & info)
	{
		torrent torr;
		//conv_type conv {stmt, torr};

		int count = stmt.column_count();
		for (int i = 0; i < count; ++i)
		{ 
			unsigned key  = std::get<0>(info[i]);
			unsigned type = std::get<1>(info[i]);

			switch (type)
			{
				case model_meta::Uint64:
				case model_meta::Speed:
				case model_meta::Size:
					torr.set_item(key, sqlite3yaw::get<optional<uint64_type>>(stmt, i));
					break;

				case model_meta::Bool:
					torr.set_item(key, sqlite3yaw::get<optional<bool>>(stmt, i));
					break;

				case model_meta::Double:
				case model_meta::Percent:
				case model_meta::Ratio:
					torr.set_item(key, sqlite3yaw::get<optional<double>>(stmt, i));
					break;

				case model_meta::String:
					torr.set_item(key, sqlite3yaw::get<optional<string_type>>(stmt, i));
					break;

				case model_meta::DateTime:
					torr.set_item(key, sqlite3yaw::get<optional<datetime_type>>(stmt, i));
					break;

				case model_meta::Duration:
					torr.set_item(key, sqlite3yaw::get<optional<duration_type>>(stmt, i));
					break;

				default:
					break;
			}
		}

		return torr;
	}

	auto load_torrents(sqlite3yaw::session & ses) -> torrent_list
	{
		auto meta = sqlite3yaw::load_table_meta(ses, torrents_table_name);
		auto info = torrents_column_info();

		boost::remove_erase_if(info, [&meta](auto & val) 
		{
			auto & name = std::get<2>(val);
			auto it = boost::find_if(meta.fields, [&name](auto & v) { return v.name == name; });
			return it == meta.fields.end();
		});

		auto cmd = sqlite3yaw::select_command(torrents_table_name, info | ext::getted<2>);
		auto stmt = ses.prepare(cmd);

		auto loader = [&info](auto & stmt) { return load_torrent(stmt, info); };
		auto recs = sqlite3yaw::make_record_range(stmt, loader);
		torrent_list result(recs.begin(), recs.end());
		return result;
	}
}
