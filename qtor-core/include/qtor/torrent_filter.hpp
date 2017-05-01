#pragma once
#include <qtor/torrent.hpp>

namespace qtor
{
	class torrent_filter
	{
		string_type m_search;

	public:
		// same, incremental
		std::pair<bool, bool> set_expr(std::string search);

		bool matches(const torrent & t) const noexcept;
		bool always_matches() const noexcept;

		bool operator()(const torrent & t) const noexcept { return matches(t); }
		explicit operator bool() const noexcept { return always_matches(); }
	};
}
