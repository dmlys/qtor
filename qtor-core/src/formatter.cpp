﻿#include <qtor/formatter.hpp>
#include <QtTools/ToolsBase.hpp>
#include <QtTools/DateUtils.hpp>
#include <QtCore/QStringBuilder>
#include <QtCore/QCoreApplication>

#define FORMATTER "formatter"

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
		QT_TRANSLATE_NOOP(FORMATTER, "%1 bytes"),
		QT_TRANSLATE_NOOP(FORMATTER, "%1 kbytes"),
		QT_TRANSLATE_NOOP(FORMATTER, "%1 Mbytes"),
		QT_TRANSLATE_NOOP(FORMATTER, "%1 Gbytes"),
		QT_TRANSLATE_NOOP(FORMATTER, "%1 Tbytes"),
	};

	const char * formatter::ms_speed_strings[5] =
	{
		QT_TRANSLATE_NOOP(FORMATTER, "%1 B/s"),
		QT_TRANSLATE_NOOP(FORMATTER, "%1 kB/s"),
		QT_TRANSLATE_NOOP(FORMATTER, "%1 MB/s"),
		QT_TRANSLATE_NOOP(FORMATTER, "%1 GB/s"),
		QT_TRANSLATE_NOOP(FORMATTER, "%1 TB/s"),
	};

	const char * formatter::ms_duration_strings[4] =
	{
		QT_TRANSLATE_NOOP(FORMATTER, "%Ln day(s)"),
		QT_TRANSLATE_NOOP(FORMATTER, "%Ln hour(s)"),
		QT_TRANSLATE_NOOP(FORMATTER, "%Ln minute(s)"),
		QT_TRANSLATE_NOOP(FORMATTER, "%Ln second(s)"),
	};

	auto formatter::weigh(double val) const noexcept -> weight
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

	QString formatter::format_uint_item(uint64_type val, const char * strings[5]) const
	{
		auto w = weigh(val);
		auto fmt = strings[w];
		double v = static_cast<double>(val) / ms_weights[w];

		unsigned precision = w == none ? 0 
		                               : v < 100 ? 2 : 1;

		return tr(fmt).arg(m_locale.toString(v, 'f', precision));
	}

	QString formatter::format_size(size_type val) const
	{
		return format_uint_item(val, ms_size_strings);
	}

	QString formatter::format_speed(size_type val) const
	{
		return format_uint_item(val, ms_speed_strings);
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

	QString formatter::format_datetime_short(datetime_type val) const
	{
		auto dt = QtTools::ToQDateTime(val);
		return m_locale.toString(dt, QLocale::FormatType::ShortFormat);
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

	QString formatter::format_duration_short(duration_type val) const
	{
		return format_duration(std::move(val));
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
		//: value has no value(std::nullopt) like does not have any speed/size information
		return tr("N/A");
	}

	QString formatter::format_unsupported() const
	{
		//: unknown data type, normally should not happen
		return tr("unknown type");
	}

	QString formatter::format_item(const any & val, unsigned type) const
	{
		switch (type)
		{
			case model_meta::Uint64   : return format_uint64(val);
			case model_meta::Int64    : return format_int64(val);
			case model_meta::Bool     : return format_bool(val);
			case model_meta::Double   : return format_double(val);
			case model_meta::String   : return format_string(val);

			case model_meta::Speed    : return format_speed(val);
			case model_meta::Size     : return format_size(val);
			case model_meta::DateTime : return format_datetime(val);
			case model_meta::Duration : return format_duration(val);
			case model_meta::Percent  : return format_percent(val);
			case model_meta::Ratio    : return format_ratio(val);

		default:
			return format_unsupported();
		}
	}

	QString formatter::format_item_short(const any & val, unsigned type) const
	{
		switch (type)
		{
			case model_meta::Uint64   : return format_uint64(val);
			case model_meta::Int64    : return format_int64(val);
			case model_meta::Bool     : return format_bool(val);
			case model_meta::Double   : return format_double(val);
			case model_meta::String   : return format_short_string(val);

			case model_meta::Speed    : return format_speed(val);
			case model_meta::Size     : return format_size(val);
			case model_meta::DateTime : return format_datetime_short(val);
			case model_meta::Duration : return format_duration_short(val);
			case model_meta::Percent  : return format_percent(val);
			case model_meta::Ratio    : return format_ratio(val);

		default:
			return format_unsupported();
		}
	}
}
