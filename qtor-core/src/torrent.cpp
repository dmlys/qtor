#include <qtor/torrent.hpp>
#include <ext/config.hpp>
#include <boost/preprocessor/stringize.hpp>

namespace qtor
{
	const string_type torrent::ms_emptystr;

#define QTOR_TORRENT_FORMATTER_ITEM(A0, ID, NAME, TYPE, TYPENAME) \
		(*types)[torrent::ID] = item {                        \
			type_map[BOOST_PP_STRINGIZE(TYPE)],               \
			QString(BOOST_PP_STRINGIZE(ID)),                  \
			&formatter_type::format_##TYPE,                   \
		};                                                    \

	const torrent_meta::item_map_ptr torrent_meta::ms_items =
		[]() -> torrent_meta::item_map_ptr
		{
			using namespace std::string_literals;
			std::unordered_map<string_type, unsigned> type_map =
			{
				{"uint64", Uint64},
				{"bool", Bool},
				{"double", Double},
				{"string", String},
				
				{"percent", Percent},
				{"ratio", Ratio},
				{"speed", Speed},
				{"size", Size},

				{"datetime", DateTime},
				{"Duration", Duration},
			};

			auto types = std::make_shared<item_map>();
			QTOR_TORRENT_FOR_EACH_FIELD(QTOR_TORRENT_FORMATTER_ITEM);

			return types;
		}();

	torrent_meta::torrent_meta(QObject * parent /* = nullptr */)
	    : formatter(parent),
	      base_type(ms_items) {}


	/************************************************************************/
	/*                  torrent_file_meta                                   */
	/************************************************************************/
	auto torrent_file_meta::item_count() const noexcept -> index_type
	{
		return torrent_file::FiledCount;
	}

	unsigned torrent_file_meta::item_type(index_type type) const noexcept
	{
		switch (type)
		{
			case torrent_file::FileName:  return sparse_container_meta::String;
			case torrent_file::TotalSize: return sparse_container_meta::Size;
			case torrent_file::HaveSize:  return sparse_container_meta::Size;
			case torrent_file::Index:     return sparse_container_meta::Int64;
			case torrent_file::Priority:  return sparse_container_meta::Int64;
			case torrent_file::Wanted:    return sparse_container_meta::Bool;

			default:
				EXT_UNREACHABLE();
		}
	}

	string_type torrent_file_meta::item_name(index_type item) const
	{
		switch (item)
		{
			case torrent_file::FileName:  return QStringLiteral("fname");
			case torrent_file::TotalSize: return QStringLiteral("total size");
			case torrent_file::HaveSize:  return QStringLiteral("have size");
			case torrent_file::Index:     return QStringLiteral("index");
			case torrent_file::Priority:  return QStringLiteral("priority");
			case torrent_file::Wanted:    return QStringLiteral("wanted");

			default:
				EXT_UNREACHABLE();
		}
	}

	QString torrent_file_meta::format_item(const torrent_file_entity & val, index_type index) const
	{
		auto visitor = [this, index](auto * ptr) -> QString { return this->format_item(*ptr, index); };
		return visit(visitor, val);
	}

	QString torrent_file_meta::format_item_short(const torrent_file_entity & val, index_type index) const
	{
		auto visitor = [this, index](auto * ptr) -> QString { return this->format_item_short(*ptr, index); };
		return visit(visitor, val);
	}

	QString torrent_file_meta::format_item(const torrent_file & val, index_type item) const
	{
		switch (item)
		{
			case torrent_file::FileName:  return format_string(val.filename);
			case torrent_file::TotalSize: return format_size(val.total_size);
			case torrent_file::HaveSize:  return format_size(val.have_size);
			case torrent_file::Index:     return format_int64(val.index);
			case torrent_file::Priority:  return format_int64(val.priority);
			case torrent_file::Wanted:    return format_bool(val.wanted);

			default:
				EXT_UNREACHABLE();
		}
	}

	QString torrent_file_meta::format_item(const torrent_dir & val, index_type item) const
	{
		switch (item)
		{
			case torrent_file::FileName:  return format_string(val.filename);
			case torrent_file::TotalSize: return format_size(val.total_size);
			case torrent_file::HaveSize:  return format_size(val.have_size);
			case torrent_file::Index:     return format_int64(val.index);
			case torrent_file::Priority:  return format_int64(val.priority);
			case torrent_file::Wanted:    return format_bool(val.wanted);

			default:
				EXT_UNREACHABLE();
		}
	}

	QString torrent_file_meta::format_item_short(const torrent_file & val, index_type item) const
	{
		switch (item)
		{
			case torrent_file::FileName:  return format_short_string(val.filename);
			case torrent_file::TotalSize: return format_size(val.total_size);
			case torrent_file::HaveSize:  return format_size(val.have_size);
			case torrent_file::Index:     return format_int64(val.index);
			case torrent_file::Priority:  return format_int64(val.priority);
			case torrent_file::Wanted:    return format_bool(val.wanted);

			default:
				EXT_UNREACHABLE();
		}
	}

	QString torrent_file_meta::format_item_short(const torrent_dir & val, index_type item) const
	{
		switch (item)
		{
			case torrent_file::FileName:  return format_short_string(val.filename);
			case torrent_file::TotalSize: return format_size(val.total_size);
			case torrent_file::HaveSize:  return format_size(val.have_size);
			case torrent_file::Index:     return format_int64(val.index);
			case torrent_file::Priority:  return format_int64(val.priority);
			case torrent_file::Wanted:    return format_bool(val.wanted);

			default:
				EXT_UNREACHABLE();
		}
	}

	torrent_file_meta::torrent_file_meta(QObject * parent /* = nullptr */)
		: formatter(parent)
	{

	}
}
