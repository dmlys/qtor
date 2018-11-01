#include <qtor/torrent.hpp>
#include <qtor/FileTreeModel.hqt>
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
		return FiledCount;
	}

	unsigned torrent_file_meta::item_type(index_type type) const noexcept
	{
		switch (type)
		{
			case FileName:
			case FilePath:  return model_meta::String;
			case TotalSize: return model_meta::Size;
			case HaveSize:  return model_meta::Size;
			case Index:     return model_meta::Int64;
			case Priority:  return model_meta::Int64;
			case Wanted:    return model_meta::Bool;

			default:
				EXT_UNREACHABLE();
		}
	}

	string_type torrent_file_meta::item_name(index_type item) const
	{
		switch (item)
		{
			case FileName:  return QStringLiteral("fname");
			case FilePath:  return QStringLiteral("fpath");
			case TotalSize: return QStringLiteral("total size");
			case HaveSize:  return QStringLiteral("have size");
			case Index:     return QStringLiteral("index");
			case Priority:  return QStringLiteral("priority");
			case Wanted:    return QStringLiteral("wanted");

			default:
				EXT_UNREACHABLE();
		}
	}

	auto torrent_file_meta::get_item(const torrent_file & item, int index) const -> any_type
	{
		switch (index)
		{
			case FileName:  return make_any(torrent_file_tree_traits::get_name(item.filename));
			case FilePath:  return make_any(item.filename);
			case TotalSize: return make_any(item.total_size);
			case HaveSize:  return make_any(item.have_size);
			case Index:     return make_any(item.index);
			case Priority:  return make_any(item.priority);
			case Wanted:    return make_any(item.wanted);

			default: return any_type();
		}
	}

	auto torrent_file_meta::get_item(const torrent_dir & item, int index) const -> any_type
	{
		switch (index)
		{
			case FileName:  return make_any(item.filename);
			case FilePath:  return make_any(item.filename);
			case TotalSize: return make_any(item.total_size);
			case HaveSize:  return make_any(item.have_size);
			case Index:     return make_any(item.index);
			case Priority:  return make_any(item.priority);
			case Wanted:    return make_any(item.wanted);

			default: return any_type();
		}
	}

	auto torrent_file_meta::get_item(const torrent_file_entity & item, int index) const -> any_type
	{
		auto visitor = [this, index](auto * ptr) { return get_item(*ptr, index); };
		return std::visit(visitor, item);
	}
}
