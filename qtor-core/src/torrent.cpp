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

}
