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

	torrent_meta::torrent_meta()
	    : base_type(ms_items)
	{

	}


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
			case torrent_file::FileName:  return model_meta::String;
			case torrent_file::TotalSize: return model_meta::Size;
			case torrent_file::HaveSize:  return model_meta::Size;
			case torrent_file::Index:     return model_meta::Int64;
			case torrent_file::Priority:  return model_meta::Int64;
			case torrent_file::Wanted:    return model_meta::Bool;

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

	auto torrent_file_meta::get_item(const torrent_file & item, int index) const -> any_type
	{
		switch (index)
		{
			case torrent_file::FileName:  return make_any(item.filename);
			case torrent_file::TotalSize: return make_any(item.total_size);
			case torrent_file::HaveSize:  return make_any(item.have_size);
			case torrent_file::Index:     return make_any(item.index);
			case torrent_file::Priority:  return make_any(item.priority);
			case torrent_file::Wanted:    return make_any(item.wanted);

			default: return any_type();
		}
	}

	auto torrent_file_meta::get_item(const torrent_dir & item, int index) const -> any_type
	{
		switch (index)
		{
			case torrent_file::FileName:  return make_any(item.filename);
			case torrent_file::TotalSize: return make_any(item.total_size);
			case torrent_file::HaveSize:  return make_any(item.have_size);
			case torrent_file::Index:     return make_any(item.index);
			case torrent_file::Priority:  return make_any(item.priority);
			case torrent_file::Wanted:    return make_any(item.wanted);

			default: return any_type();
		}
	}

	auto torrent_file_meta::get_item(const torrent_file_entity & item, int index) const -> any_type
	{
		auto visitor = [this, index](auto * ptr) { return get_item(*ptr, index); };
		return std::visit(visitor, item);
	}
}
