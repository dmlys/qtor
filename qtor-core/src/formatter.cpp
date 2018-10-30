#include <qtor/formatter.hqt>
#include <QtTools/ToolsBase.hpp>
#include <QtTools/DateUtils.hpp>
#include <QtCore/QStringBuilder>

namespace qtor
{
	const std::array<uint64_type, 5> formatter::ms_weights =
	{
		1,                                // none
		1'000ull,                         // kilo
		1'000ull * 1'000,                 // mega
		1'000ull * 1'000 * 1'000,         // giga
		1'000ull * 1'000 * 1'000 * 1'000, // tera
	};

	const char * formatter::ms_size_strings[5] =
	{
		QT_TRANSLATE_NOOP("formatter", "%1 bytes"),
		QT_TRANSLATE_NOOP("formatter", "%1 kbytes"),
		QT_TRANSLATE_NOOP("formatter", "%1 Mbytes"),
		QT_TRANSLATE_NOOP("formatter", "%1 Gbytes"),
		QT_TRANSLATE_NOOP("formatter", "%1 Tbytes"),
	};

	const char * formatter::ms_speed_strings[5] =
	{
		QT_TRANSLATE_NOOP("formatter", "%1 B/s"),
		QT_TRANSLATE_NOOP("formatter", "%1 kB/s"),
		QT_TRANSLATE_NOOP("formatter", "%1 MB/s"),
		QT_TRANSLATE_NOOP("formatter", "%1 GB/s"),
		QT_TRANSLATE_NOOP("formatter", "%1 TB/s"),
	};

	const char * formatter::ms_duration_strings[4] =
	{
		QT_TRANSLATE_NOOP("formatter", "%Ln day(s)"),
		QT_TRANSLATE_NOOP("formatter", "%Ln hour(s)"),
		QT_TRANSLATE_NOOP("formatter", "%Ln minute(s)"),
		QT_TRANSLATE_NOOP("formatter", "%Ln second(s)"),
	};

	auto formatter::weigh(double  val) const noexcept -> weight
	{
		auto first = ms_weights.begin();
		auto last  = ms_weights.end();
		auto it = std::upper_bound(first, last, val);
		return static_cast<weight>(std::max<std::ptrdiff_t>(it - first - 1, 0));
	}

	auto formatter::weigh(double val, weight w) const noexcept -> double
	{
		return val * ms_weights[w];
	}

	QString formatter::format_item(uint64_type val, const char * strings[5]) const
	{
		auto w = weigh(val);
		auto fmt = strings[w];
		double v = 1.0 * val / ms_weights[w];

		unsigned precision = w == none ? 0 
		                               : v < 100 ? 2 : 1;

		return tr(fmt).arg(m_locale.toString(v, 'f', precision));
	}

	QString formatter::format_size(size_type val) const
	{
		return format_item(val, ms_size_strings);
	}

	QString formatter::format_speed(size_type val) const
	{
		return format_item(val, ms_speed_strings);
	}

	QString formatter::format_int64(int64_type val) const
	{
		return m_locale.toString(static_cast<qlonglong>(val));
	}

	QString formatter::format_uint64(uint64_type val) const
	{
		return m_locale.toString(static_cast<qulonglong>(val));
	}

	QString formatter::format_double(double val) const
	{
		return m_locale.toString(val, 'f', 2);
	}

	QString formatter::format_string(string_type val) const
	{
		return ToQString(val);
	}

	QString formatter::format_short_string(string_type val) const
	{
		return ToQString(val);
	}

	QString formatter::format_datetime(datetime_type val) const
	{
		auto dt = QtTools::ToQDateTime(val);
		return m_locale.toString(dt);
	}

	QString formatter::format_duration(duration_type val) const
	{
		uint64_type secs = std::chrono::duration_cast<std::chrono::seconds>(val).count();

		constexpr auto min_sec = 60;
		constexpr auto hour_sec = min_sec * 60;
		constexpr auto day_sec = hour_sec * 24;

		uint64_type vals[4] =
		{
			secs / day_sec,             // days
			secs % day_sec  / hour_sec, // hours
			secs % hour_sec / min_sec,  // minutes
			secs % min_sec,             // secs
		};

		auto first = std::begin(vals);
		auto last = std::end(vals) - 1;
		auto it = std::find_if(first, last, [](auto v) { return v != 0; });
		auto idx = it - first;

		// only seconds
		if (it == last) return tr(ms_duration_strings[idx], nullptr, *it);

		if (*it >= 4) return tr(ms_duration_strings[idx], nullptr, *it);
		
		return tr(ms_duration_strings[idx], nullptr, *it)
		     % QStringLiteral(", ")
		     % tr(ms_duration_strings[idx + 1], nullptr, *(it + 1));
	}

	QString formatter::format_ratio(double val) const
	{
		return format_double(val);
	}

	QString formatter::format_percent(double val) const
	{
		//assert(0.0 <= val and val <= 1.0);
		return format_double(val * 100) + m_locale.percent();
	}

	QString formatter::format_bool(bool val) const
	{
		return m_locale.toString(val);
	}

	QString formatter::format_nullopt() const
	{
		return QStringLiteral("N/A");
	}
}
