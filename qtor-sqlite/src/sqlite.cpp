#include <qtor/sqlite.hpp>
#include <qtor/sqlite-conv-qtor.hpp>
#include <qtor/utils.hpp>

#include <qtor/types.hpp>
#include <qtor/model_meta.hpp>
#include <qtor/bound_meta.hpp>
#include <qtor/torrent.hpp>
#include <qtor/torrent_file.hpp>

#include <boost/range/counting_range.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace qtor::sqlite
{
	const std::string torrents_table_name = "torrents";
	const std::string torrent_files_table_name = "torrent_files";

	struct field_info
	{
		std::string name;
		std::string_view sqltype;
		unsigned type;
		unsigned index;
	};

	static const char * get_sql_type(unsigned type)
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

	static auto create_info(const model_meta & meta)
	{
		auto make_info = [&meta](auto index)
		{
			auto type = meta.item_type(index);
			return field_info {
				FromQString(meta.item_name(index)),
				get_sql_type(type),
				type,
				index,
			};
		};

		auto count = meta.item_count();
		auto fields = boost::counting_range(0u, count) | boost::adaptors::transformed(make_info);
		return std::vector(fields.begin(), fields.end());
	}

	void create_table(sqlite3yaw::session & ses, const std::string & name, const model_meta & meta)
	{
		std::string cmd;
		cmd.resize(1024);

		cmd = "create table if not exists ";
		sqlite3yaw::escape_sql_name(name, cmd);
		cmd += "( ";

		for (auto & field : create_info(meta))
		{
			auto name = field.name;
			auto type = field.sqltype;
			sqlite3yaw::escape_sql_name(name, cmd);

			cmd += ' ';
			cmd += type;

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
		return create_table(ses, torrents_table_name, default_torrent_meta());
	}

	void drop_torrents_table(sqlite3yaw::session & ses)
	{
		std::string cmd = "drop table ";
		sqlite3yaw::escape_sql_name(torrents_table_name, cmd);

		ses.exec_ex(cmd);
	}

	void create_torrent_files_table(sqlite3yaw::session & ses)
	{
		return create_table(ses, torrent_files_table_name, torrent_file_meta());
	}

	void drop_torrent_files_table(sqlite3yaw::session & ses)
	{
		std::string cmd = "drop table ";
		sqlite3yaw::escape_sql_name(torrent_files_table_name);

		ses.exec_ex(cmd);
	}


	void save_torrents(sqlite3yaw::session & ses, const torrent_list & torrents)
	{
		auto & meta = default_torrent_meta();
		auto tmeta = sqlite3yaw::load_table_meta(ses, torrents_table_name);

		auto field_info = create_info(meta);
		auto names = field_info | boost::adaptors::transformed(std::mem_fn(&field_info::name));

		auto batch_range = make_batch_range(meta, names, torrents);
		sqlite3yaw::batch_upsert(batch_range, ses, tmeta);
	}

	static torrent load_torrent(sqlite3yaw::statement & stmt, const std::vector<field_info> & meta)
	{
		torrent torr;
		//conv_type conv {stmt, torr};

		for (auto & info : meta)
		{ 
			unsigned key  = info.index;
			unsigned type = info.type;

			switch (type)
			{
				case model_meta::Uint64:
				case model_meta::Speed:
				case model_meta::Size:
					torr.set_item(key, sqlite3yaw::get<optional<uint64_type>>(stmt, key));
					break;

				case model_meta::Bool:
					torr.set_item(key, sqlite3yaw::get<optional<bool>>(stmt, key));
					break;

				case model_meta::Double:
				case model_meta::Percent:
				case model_meta::Ratio:
					torr.set_item(key, sqlite3yaw::get<optional<double>>(stmt, key));
					break;

				case model_meta::String:
					torr.set_item(key, sqlite3yaw::get<optional<string_type>>(stmt, key));
					break;

				case model_meta::DateTime:
					torr.set_item(key, sqlite3yaw::get<optional<datetime_type>>(stmt, key));
					break;

				case model_meta::Duration:
					torr.set_item(key, sqlite3yaw::get<optional<duration_type>>(stmt, key));
					break;

				default:
					break;
			}
		}

		return torr;
	}

	auto load_torrents(sqlite3yaw::session & ses) -> torrent_list
	{
		auto & meta = default_torrent_meta();
		auto info = create_info(meta);
		auto tmeta = sqlite3yaw::load_table_meta(ses, torrents_table_name);

		boost::remove_erase_if(info, [&tmeta](auto & val)
		{
			auto & name = val.name;
			auto it = boost::find_if(tmeta.fields, [&name](auto & v) { return v.name == name; });
			return it == tmeta.fields.end();
		});

		auto names = info | boost::adaptors::transformed(std::mem_fn(&field_info::name));
		auto cmd = sqlite3yaw::select_command(torrents_table_name, names);
		auto stmt = ses.prepare(cmd);

		auto loader = [&info](auto & stmt) { return load_torrent(stmt, info); };
		auto recs = sqlite3yaw::make_record_range(stmt, loader);
		torrent_list result(recs.begin(), recs.end());
		return result;
	}

	void save_torrent_files(sqlite3yaw::session & ses, const torrent_file_list & files)
	{
		using namespace std;
		const auto & meta = torrent_file_meta::instance;
		auto tmeta = sqlite3yaw::load_table_meta(ses, torrents_table_name);

		torrent_meta_adapter<torrent_file> adapter(meta);

		auto field_info = create_info(adapter);
		auto names = field_info | boost::adaptors::transformed(std::mem_fn(&field_info::name));

		auto batch_range = make_batch_range(adapter, names, files);
		sqlite3yaw::batch_upsert(batch_range, ses, tmeta);
	}
}
