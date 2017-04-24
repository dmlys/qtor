#pragma once
#include <functional>
#include <boost/variant.hpp>
#include <qtor/torrent.hpp>

namespace qtor
{
	template <class relation_comparator>
	struct torrent_id_comparator : relation_comparator
	{
		bool operator()(const torrent & t1, const torrent & t2) const noexcept
		{
			return relation_comparator::operator() (t1.id, t2.id);
		}
	};

	template <class relation_comparator>
	struct torrent_name_comparator : relation_comparator
	{
		bool operator()(const torrent & t1, const torrent & t2) const noexcept
		{
			return relation_comparator::operator() (t1.name, t2.name);
		}
	};

	typedef torrent_id_comparator<std::less<>>    torrent_id_less;
	typedef torrent_id_comparator<std::greater<>> torrent_id_greater;

	typedef torrent_name_comparator<std::less<>>    torrent_name_less;
	typedef torrent_name_comparator<std::greater<>> torrent_name_greater;

	typedef boost::variant<
		torrent_id_less, torrent_id_greater,
		torrent_name_less, torrent_name_greater
	> torrent_comparator;
}
