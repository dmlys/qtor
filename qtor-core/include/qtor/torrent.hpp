#pragma once
#include <cstddef>
#include <utility>
#include <functional>
#include <chrono>
#include <string>
#include <vector>

namespace qtor
{
	typedef std::string   string_type;
	typedef std::int64_t  speed_type;
	typedef std::int64_t  size_type;

	typedef std::chrono::system_clock::time_point  time_point_type;
	typedef std::chrono::system_clock::duration    duration_type;

	struct torrent;
	struct torrent_filename;
	struct statistics;

	typedef string_type                      torrent_id_type;
	typedef std::vector<torrent_id_type>     torrent_id_list;
	typedef std::vector<torrent>             torrent_list;

	struct torrent
	{
		torrent_id_type id;		
		string_type     name;

		size_type    total_size;
		speed_type   download_speed;
		speed_type   upload_speed;
		
		time_point_type date_added;
		time_point_type date_created;
		duration_type  eta;


		/// those fields are exposed as columns in a table model
		enum
		{
			Name,
			TotalSize,
			DownloadSpeed,
			UploadSpeed,
			DateAdded,
			DateCreated,
		};
	};

	struct torrent_id_hasher
	{
		auto operator()(const torrent & val) const noexcept
		{
			return std::hash<torrent_id_type>{} (val.id);
		}
	};

	struct torrent_id_equal
	{
		bool operator()(const torrent & t1, const torrent & t2) const noexcept
		{
			return t1.id == t2.id;
		}
	};

	template <class relation_comparator>
	struct torrent_id_comparator : relation_comparator
	{
		bool operator()(const torrent & t1, const torrent & t2) const noexcept
		{
			return relation_comparator::operator() (t1.id, t2.id);
		}
	};

	typedef torrent_id_comparator<std::less<>>    torrent_id_less;
	typedef torrent_id_comparator<std::greater<>> torrent_id_greater;
}
