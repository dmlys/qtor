#pragma once
#include <cstddef>
#include <memory>
#include <array>
#include <qtor/types.hpp>
#include <qtor/model_meta.hpp>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QLocale>
#include <QtCore/QCoreApplication>

namespace qtor
{
	class formatter
	{
		Q_DECLARE_TR_FUNCTIONS("formatter")

	public:
		// weights
		enum weight : unsigned
		{
			none, kilo, mega, giga, tera
		};

	protected:
		static const std::array<uint64_type, 5> ms_weights;
		static const char * ms_size_strings[5];
		static const char * ms_speed_strings[5];
		static const char * ms_duration_strings[4];

		QLocale m_locale;

	public:
		void set_locale(QLocale loc) { m_locale = std::move(loc); }
		auto get_locale() const noexcept { return m_locale; }

	public:
		virtual auto weigh(double val) const noexcept -> weight;
		virtual auto weigh(double val, weight w) const noexcept -> double;
		virtual QString format_uint_item(uint64_type val, const char *strings[5]) const;

	public:
		// more specialized formatters
		virtual QString format_size(size_type val) const;
		virtual QString format_speed(speed_type val) const;
		
		// more generic formatters
		virtual QString format_int64(int64_type val) const;
		virtual QString format_uint64(uint64_type val) const;
		virtual QString format_double(double val) const;
		virtual QString format_string(string_type val) const;
		virtual QString format_short_string(string_type val) const;
		
		virtual QString format_datetime(datetime_type val) const;
		virtual QString format_duration(duration_type val) const;

		virtual QString format_datetime_short(datetime_type val) const;
		virtual QString format_duration_short(duration_type val) const;

		virtual QString format_ratio(double val) const;
		virtual QString format_percent(double val) const;
		virtual QString format_bool(bool val) const;
		virtual QString format_nullopt() const;
		virtual QString format_unsupported() const;

	public:
		/// type is same as defined in model_meta
		virtual QString format_item(const any & val, unsigned type) const;
		virtual QString format_item_short(const any & val, unsigned type) const;

	public:
#define QTOR_FORMATTER_OPTIONAL_ANY_OVERLOAD(NAME, TYPE)                              \
		template <class Type>                                                         \
		std::enable_if_t<std::is_convertible_v<std::decay_t<Type>, TYPE>, QString>    \
		NAME(const optional<Type> & val) const                                        \
		{                                                                             \
			return val ? NAME(val.value()) : format_nullopt();                        \
		}                                                                             \
		                                                                              \
		QString NAME(const any & val) const                                           \
		{                                                                             \
			const auto * ptr = any_cast<TYPE>(&val);                                  \
			return ptr ? NAME(*ptr) : format_nullopt();                               \
		}                                                                             \


		QTOR_FORMATTER_OPTIONAL_ANY_OVERLOAD(format_speed,  speed_type);
		QTOR_FORMATTER_OPTIONAL_ANY_OVERLOAD(format_size,   size_type);
		
		QTOR_FORMATTER_OPTIONAL_ANY_OVERLOAD(format_bool, bool);
		QTOR_FORMATTER_OPTIONAL_ANY_OVERLOAD(format_uint64, uint64_type);
		QTOR_FORMATTER_OPTIONAL_ANY_OVERLOAD(format_int64, int64_type);
		QTOR_FORMATTER_OPTIONAL_ANY_OVERLOAD(format_double, double);
		QTOR_FORMATTER_OPTIONAL_ANY_OVERLOAD(format_string, string_type);
		QTOR_FORMATTER_OPTIONAL_ANY_OVERLOAD(format_short_string, string_type);

		QTOR_FORMATTER_OPTIONAL_ANY_OVERLOAD(format_datetime, datetime_type);
		QTOR_FORMATTER_OPTIONAL_ANY_OVERLOAD(format_duration, duration_type);

		QTOR_FORMATTER_OPTIONAL_ANY_OVERLOAD(format_datetime_short, datetime_type);
		QTOR_FORMATTER_OPTIONAL_ANY_OVERLOAD(format_duration_short, duration_type);

		QTOR_FORMATTER_OPTIONAL_ANY_OVERLOAD(format_percent, double);
		QTOR_FORMATTER_OPTIONAL_ANY_OVERLOAD(format_ratio,   double);

#undef QTOR_FORMATTER_OPTIONAL_ANY_OVERLOAD

	public:
		virtual ~formatter() = default;
	};
}

Q_DECLARE_METATYPE(qtor::formatter *);
