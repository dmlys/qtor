#pragma once
#include <qtor/torrent.hpp>

namespace qtor
{
	struct torrent_filter
	{
		bool matches(const torrent & t) const noexcept { return true; }
		bool always_matches() const noexcept { return true; }

		bool operator()(const torrent & t) const noexcept { return matches(t); }
		explicit operator bool() const noexcept { return always_matches(); }
	};
}
