#pragma once
#include <cstddef>
#include <cstdlib>
#include <climits>
#include <limits>
#include <string>
#include <charconv>

#include <ext/itoa.hpp>

#include <QtCore/QString>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>

#include <QtTools/ToolsBase.hpp>
#include <QtTools/DateUtils.hpp>

namespace QtTools
{
	namespace register_helpers
	{
		template <class SignedType, class StringType>
		std::enable_if_t<std::is_signed_v<SignedType>, SignedType> to_integral(const StringType & str)
		{
			const char * first = str.data();

			long long result = std::strtoll(first, nullptr, 10);
			constexpr long long min = std::numeric_limits<SignedType>::min();
			constexpr long long max = std::numeric_limits<SignedType>::max();
			return static_cast<SignedType>(std::clamp(min, result, max));
		}

		template <class UnsignedType, class StringType>
		std::enable_if_t<std::is_unsigned_v<UnsignedType>, UnsignedType> to_integral(const StringType & str)
		{
			const char * first = str.data();

			unsigned long long result = std::strtoull(first, nullptr, 10);
			constexpr unsigned long long min = std::numeric_limits<UnsignedType>::min();
			constexpr unsigned long long max = std::numeric_limits<UnsignedType>::max();
			return static_cast<UnsignedType>(std::clamp(min, result, max));
		}

		template <class StringType, class IntegralType>
		StringType from_integral(IntegralType val)
		{
			static_assert(std::is_integral_v<IntegralType>);

			ext::itoa_buffer<IntegralType> buffer;
			auto * last = std::end(buffer) - 1;
			auto * first = ext::itoa(val, buffer);
			return StringType(first, last);
		}

		template <class Double, class StringType>
		Double to_double(const StringType & str)
		{
			const char * first = str.data();
			if constexpr (std::is_same_v<Double, float>)
				return std::strtof(first, nullptr);
			else if constexpr (std::is_same_v<Double, double>)
				return std::strtod(first, nullptr);
		    else if constexpr (std::is_same_v<Double, long double>)
				return std::strtold(first, nullptr);
		}

		template <class StringType, class FloatType>
		StringType from_double(FloatType val)
		{
			QString qstr;
			qstr.setNum(val);
			return FromQString(qstr);
		}

		template <class StringType, class DateType>
		StringType from_datetime(const DateType & datetime)
		{
			if constexpr (std::is_same_v<DateType, std::chrono::system_clock::time_point>)
			{
				auto qdt = ToQDateTime(datetime);
				auto qstr = qdt.toString(Qt::DateFormat::ISODateWithMs);
				return FromQString(qstr);
			}
			else
			{
				auto qstr = datetime.toString(Qt::DateFormat::ISODateWithMs);
				return FromQString(qstr);
			}
		}

		template <class DateType, class StringType>
		DateType to_datetime(const StringType & str)
		{
			if constexpr (std::is_same_v<DateType, std::chrono::system_clock::time_point>)
			{
				auto qstr = ToQString(str);
				return ToStdChrono(QDateTime::fromString(qstr, Qt::DateFormat::ISODateWithMs));
			}
			else
			{
				auto qstr = ToQString(str);
				return DateType::fromString(qstr, Qt::DateFormat::ISODateWithMs);
			}
		}

		template <class StringType>
		QByteArray to_bytearray(const StringType & str)
		{
			const char * first = str.data();
			int size = static_cast<int>(str.size());
			return QByteArray(first, size);
		}

		template <class StringType>
		StringType from_bytearray(const QByteArray & val)
		{
			const char * first = val.data();
			const char * last = first + val.size();
			return StringType(first, last);
		}
	}

	template <class String>
	void register_string_metatype()
	{
		using string_type = String;
		using namespace register_helpers;

		qRegisterMetaType<string_type>();
		QMetaType::registerComparators<string_type>();

		QMetaType::registerConverter<string_type, QString>(static_cast<QString(*)(const string_type & )>(ToQString));
		QMetaType::registerConverter<QString, string_type>(static_cast<string_type(*)(const QString & )>(FromQString<string_type>));

		QMetaType::registerConverter<string_type, QByteArray>(to_bytearray<string_type>);
		QMetaType::registerConverter<QByteArray, string_type>(from_bytearray<string_type>);

		#define REGISTER_NUM_CONV(type) \
	        QMetaType::registerConverter<string_type, type>(to_integral<type, string_type>); \
	        QMetaType::registerConverter<type, string_type>(from_integral<string_type, type>);

		#define REGISTER_FLOAT_CONV(type) \
			QMetaType::registerConverter<string_type, type>(to_double<type, string_type>); \
			QMetaType::registerConverter<type, string_type>(from_double<string_type, type>);

		#define REGISTER_DATE_CONV(type) \
			QMetaType::registerConverter<string_type, type>(to_datetime<type, string_type>); \
			QMetaType::registerConverter<type, string_type>(from_datetime<string_type, type>);


		REGISTER_NUM_CONV(signed short);
		REGISTER_NUM_CONV(unsigned short);
		REGISTER_NUM_CONV(signed int);
		REGISTER_NUM_CONV(unsigned int);
		REGISTER_NUM_CONV(signed long);
		REGISTER_NUM_CONV(unsigned long);
		REGISTER_NUM_CONV(signed long long);
		REGISTER_NUM_CONV(unsigned long long);

		REGISTER_FLOAT_CONV(float);
		REGISTER_FLOAT_CONV(double);
		//REGISTER_FLOAT_CONV(long double);

		REGISTER_DATE_CONV(QDateTime);
		REGISTER_DATE_CONV(QDate);
		REGISTER_DATE_CONV(QTime);
		REGISTER_DATE_CONV(std::chrono::system_clock::time_point);


		#undef REGISTER_NUM_CONV
		#undef REGISTER_FLOAT_CONV
		#undef REGISTER_DATE_CONV
	}
}
