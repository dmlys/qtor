#include <qtor/sqlite.hpp>
#include <qtor/sqlite-conv-qtor.hpp>
#include <qtor/utils.hpp>

#include <qtor/types.hpp>
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

	template <class Type>
	class torrent_meta_adapter : public model_accessor<Type>
	{
		using base_type = model_accessor<Type>;

	public:
		using typename base_type::index_type;
		using typename base_type::any_type;

	public:
		const model_accessor<Type> * wrapped;
		torrent_id_type torrent_id;

		// model_meta interface
	public:
		virtual index_type item_count() const noexcept override { return wrapped->item_count() + 1; }
		virtual unsigned item_type(index_type index) const noexcept override;
		virtual string_type item_name(index_type index) const override;
		virtual bool is_virtual_item(index_type index) const override;

		virtual any_type get_item(const Type & item, index_type index) const override;
		virtual void     set_item(Type & item, index_type index, const any_type & val) const override;

	public:
		torrent_meta_adapter(const model_accessor<Type> & wrapped, torrent_id_type id)
		    : wrapped(&wrapped), torrent_id(std::move(id)) {}
	};

	/// constructor template deduction guide
	template <class Type>
	torrent_meta_adapter(const model_accessor<Type> & wrapped, torrent_id_type) -> torrent_meta_adapter<Type>;

	template <>
	class torrent_meta_adapter<torrent_file> : public model_accessor<torrent_file>
	{
		using base_type = model_accessor<torrent_file>;

	public:
		using typename base_type::index_type;
		using typename base_type::any_type;

	public:
		const model_accessor<torrent_file> * wrapped;
		torrent_id_type torrent_id;

	public:
		virtual index_type item_count() const noexcept override { return wrapped->item_count() + 1; }
		virtual unsigned item_type(index_type index) const noexcept override;
		virtual string_type item_name(index_type index) const override;
		virtual bool is_virtual_item(index_type index) const override;

		virtual any_type get_item(const torrent_file & item, index_type index) const override;
		virtual any_type get_item(const torrent_dir & item, index_type index) const override;

		virtual void set_item(torrent_file & item, index_type index, const any_type & val) const override;
		virtual void set_item(torrent_dir & item, index_type index, const any_type & val) const override;

	public:
		torrent_meta_adapter(const model_accessor<torrent_file> & wrapped, torrent_id_type id)
		    : wrapped(&wrapped), torrent_id(std::move(id)) {}
	};


	template <class Type>
	unsigned torrent_meta_adapter<Type>::item_type(index_type index) const noexcept
	{
		if (index == 0) return model_meta::String;

		return wrapped->item_type(index - 1);
	}

	template <class Type>
	string_type torrent_meta_adapter<Type>::item_name(index_type index) const
	{
		if (index == 0) return "torrent_id";

		return wrapped->item_name(index - 1);
	}

	template <class Type>
	bool torrent_meta_adapter<Type>::is_virtual_item(index_type index) const
	{
		if (index == 0) return true;

		return base_type::is_virtual_item(index);
	}

	template <class Type>
	auto torrent_meta_adapter<Type>::get_item(const Type & item, index_type index) const -> any_type
	{
		if (index == 0) return;
		return wrapped->get_item(item, index - 1);
	}

	template <class Type>
	void torrent_meta_adapter<Type>::set_item(Type & item, index_type index, const any_type & val) const
	{
		if (index == 0) return;
		return wrapped->get_item(item, index - 1);
	}

	unsigned torrent_meta_adapter<torrent_file>::item_type(index_type key) const noexcept
	{
		if (key == 0) return model_meta::String;

		return wrapped->item_type(key - 1);
	}

	string_type torrent_meta_adapter<torrent_file>::item_name(index_type key) const
	{
		if (key == 0) return "torrent_id";

		return wrapped->item_name(key - 1);
	}

	bool torrent_meta_adapter<torrent_file>::is_virtual_item(index_type index) const
	{
		if (index == 0) return true;

		return wrapped->is_virtual_item(index);
	}

	auto torrent_meta_adapter<torrent_file>::get_item(const torrent_file & item, index_type key) const -> any_type
	{
		if (key == 0) return torrent_id;

		return wrapped->get_item(item, key - 1);
	}

	auto torrent_meta_adapter<torrent_file>::get_item(const torrent_dir & item, index_type key) const -> any_type
	{
		if (key == 0) return torrent_id;

		return wrapped->get_item(item, key - 1);
	}

	void torrent_meta_adapter<torrent_file>::set_item(torrent_file & item, index_type key, const any_type & val) const
	{
		if (key == 0) return;

		return wrapped->set_item(item, key - 1, val);
	}

	void torrent_meta_adapter<torrent_file>::set_item(torrent_dir & item, index_type key, const any_type & val) const
	{
		if (key == 0) return;

		return wrapped->set_item(item, key - 1, val);
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

	const model_accessor<sparse_container> & torrents_meta()
	{
		static const qtor::torrent_meta meta;
		return meta;
	}

	const model_accessor<torrent_file> & torrent_files_meta()
	{
		static const qtor::torrent_file_meta meta;
		return meta;
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
		return create_table(ses, torrents_table_name, torrents_meta());
	}

	void drop_torrents_table(sqlite3yaw::session & ses)
	{
		std::string cmd = "drop table ";
		sqlite3yaw::escape_sql_name(torrents_table_name, cmd);

		ses.exec_ex(cmd);
	}

	void create_torrent_files_table(sqlite3yaw::session & ses)
	{
		return create_table(ses, torrent_files_table_name, torrent_files_meta());
	}

	void drop_torrent_files_table(sqlite3yaw::session & ses)
	{
		std::string cmd = "drop table ";
		sqlite3yaw::escape_sql_name(torrent_files_table_name);

		ses.exec_ex(cmd);
	}


	void save_torrents(sqlite3yaw::session & ses, const torrent_list & torrents)
	{
		auto & meta = torrents_meta();
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
		auto & meta = torrents_meta();
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
		const auto & meta = torrent_files_meta();
		auto tmeta = sqlite3yaw::load_table_meta(ses, torrents_table_name);

		torrent_meta_adapter<torrent_file> adapter(meta, "");

		auto field_info = create_info(adapter);
		auto names = field_info | boost::adaptors::transformed(std::mem_fn(&field_info::name));

		auto batch_range = make_batch_range(meta, names, files);
//		sqlite3yaw::batch_upsert(batch_range, ses, tmeta);
	}
}
