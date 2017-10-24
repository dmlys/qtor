#include <qtor/sqlite.hpp>
#include <qtor/sqlite-conv-qtor.hpp>
#include <qtor/utils.hpp>

#include <ext/range/adaptors/getted.hpp>

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
			auto & meta = torrents_meta();

			torrent::index_type first = torrent::FirstField;
			torrent::index_type last = torrent::LastField;
			info.resize(last - first);

			for (; first != last; ++first)
			{
				auto & val = info[first];
				auto name = meta.item_name(first);
				auto type = meta.item_type(first);
				val = std::make_tuple(first, type, FromQString(name));
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

		ses.exec(cmd);
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

//	struct conv_type
//	{
//		unsigned idx = 0;
//		sqlite3yaw::statement * stmt;
//		torrent * torr;
//
//#define OPERATOR(Type)                                              \
//		void operator()(torrent & (torrent::*pmf)(Type val))        \
//		{                                                           \
//			Type val;                                               \
//			sqlite3yaw::get(*stmt, idx++, val);                     \
//			(torr->*pmf)(std::move(val));                           \
//		}                                                           \
//		
//		OPERATOR(string_type);
//		OPERATOR(uint64_type);
//		OPERATOR(double);
//		OPERATOR(bool);
//		OPERATOR(datetime_type);
//		OPERATOR(duration_type);
//		
//#undef OPERATOR
//
//		conv_type(sqlite3yaw::statement & stmt, torrent & torr)
//			: stmt(&stmt), torr(&torr) {}
//	};

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
				case sparse_container_meta::Uint64:
				case sparse_container_meta::Speed:
				case sparse_container_meta::Size:
					torr.set_item(key, sqlite3yaw::get<uint64_type>(stmt, i));
					break;

				case sparse_container_meta::Bool:
					torr.set_item(key, sqlite3yaw::get<bool>(stmt, i));
					break;

				case sparse_container_meta::Double:
				case sparse_container_meta::Percent:
				case sparse_container_meta::Ratio:
					torr.set_item(key, sqlite3yaw::get<double>(stmt, i));
					break;

				case sparse_container_meta::String:
					torr.set_item(key, sqlite3yaw::get<string_type>(stmt, i));
					break;

				case sparse_container_meta::DateTime:
					torr.set_item(key, sqlite3yaw::get<datetime_type>(stmt, i));
					break;

				case sparse_container_meta::Duration:
					torr.set_item(key, sqlite3yaw::get<duration_type>(stmt, i));
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
