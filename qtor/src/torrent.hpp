#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include <utility>

namespace qtor
{
	struct torrent;
	struct torrent_filename;
	struct statistics;

	typedef std::string                      torrent_index_type;
	typedef std::vector<torrent_index_type>  torrent_index_list;
	typedef std::vector<torrent>             torrent_list;

	struct torrent
	{
		torrent_index_type id;
		
		std::string name;
	};


	struct torrent_id_hasher
	{
		auto operator()(const torrent & val) const noexcept
		{
			return std::hash<torrent_index_type>{} (val.id);
		}
	};

	struct torrent_id_equal
	{
		bool operator()(const torrent & t1, const torrent & t2) const noexcept
		{
			return t1.id == t2.id;
		}
	};
}
