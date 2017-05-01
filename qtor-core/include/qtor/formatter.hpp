#pragma once
#include <cstddef>
#include <QtCore/QString>
#include <qtor/torrent.hpp>

namespace qtor
{
	class formatter
	{
	public:
		QString format_int64(std::int64_t val) const;
		QString format_speed(speed_type val) const;
		QString format_size(size_type val) const;

		QString format_string(string_type val) const;
		QString format_short_string(string_type val) const;
	};
}
