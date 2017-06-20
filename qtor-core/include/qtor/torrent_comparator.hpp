#pragma once
#include <boost/variant.hpp>
#include <ext/config.hpp>

#include <qtor/torrent.hpp>
#include <qtor/AbstractTorrentModel.hqt>

namespace qtor
{
	template <class type>
	struct torrent_type_comparator
	{
		type torrent::*field;
		bool descending;

		bool operator()(const torrent & t1, const torrent & t2) const noexcept
		{
			return descending ^ (t1.*field < t2.*field);
		}

		torrent_type_comparator(type torrent::*field, bool ascending = true)
			: field(field), descending(!ascending) {}
	};

	typedef torrent_type_comparator<string_type>      torrent_string_comparator;
	typedef torrent_type_comparator<time_point_type>  torrent_time_point_comparator;
	typedef torrent_type_comparator<std::uint64_t>    torrent_uint64_comparator;	

	typedef boost::variant<
		torrent_id_less,
		torrent_string_comparator,
		torrent_time_point_comparator, 
		torrent_uint64_comparator
	> torrent_comparator;


	template <class Type>
	torrent_comparator create_comparator(Type torrent::*field, bool ascending = true)
	{
		return torrent_type_comparator<Type> (field, ascending);
	}

	inline torrent_comparator create_comparator(unsigned type, bool ascending = true)
	{
		switch (type)
		{			
			case AbstractTorrentModel::Name:          return create_comparator(&torrent::name             , ascending);
			case AbstractTorrentModel::TotalSize:     return create_comparator(&torrent::total_size       , ascending);
			case AbstractTorrentModel::DownloadSpeed: return create_comparator(&torrent::download_speed   , ascending);
			case AbstractTorrentModel::UploadSpeed:   return create_comparator(&torrent::upload_speed     , ascending);
			case AbstractTorrentModel::DateAdded:     return create_comparator(&torrent::date_added       , ascending);
			case AbstractTorrentModel::DateCreated:   return create_comparator(&torrent::date_created     , ascending);

			default:
				EXT_UNREACHABLE();
		}
	}
}
