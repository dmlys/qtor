#pragma once
#include <cstddef>
#include <cstdlib>
#include <climits>
#include <limits>
#include <string>

#include <ext/itoa.hpp>

#include <QtCore/QString>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>

#include <QtTools/ToolsBase.hpp>

namespace QtTools
{
	template <class String>
	struct string_qvariant_converters
	{
		using string_type = String;

		static QString string_to_qstring(const string_type & str) { return QtTools::ToQString(str); }
		static string_type qstring_to_string(const QString & str) { return QtTools::FromQString(str); }

		template <class SignedType> static SignedType string_to_signed(const string_type & str);
		template <class SignedType> static SignedType string_to_unsigned(const string_type & str);
		template <class IntegralType> static string_type integral_to_string(IntegralType val);

		static auto string_to_short(const string_type & str) { return string_to_signed<signed short>(str); }
		static auto string_to_ushort(const string_type & str) { return string_to_unsigned<unsigned short>(str); }
		static auto string_to_int(const string_type & str) { return string_to_signed<signed int>(str); }
		static auto string_to_uint(const string_type & str) { return string_to_unsigned<unsigned int>(str); }
		static auto string_to_long(const string_type & str) { return string_to_signed<signed long>(str); }
		static auto string_to_ulong(const string_type & str) { return string_to_unsigned<unsigned long>(str); }
		static auto string_to_longlong(const string_type & str) { return string_to_signed<signed long long>(str); }
		static auto string_to_ulonglong(const string_type & str) { return string_to_unsigned<unsigned long long>(str); }

		static auto short_to_string(signed short val) { return integral_to_string(val); }
		static auto ushort_to_string(unsigned short val) { return integral_to_string(val); }
		static auto int_to_string(signed int val) { return integral_to_string(val); }
		static auto uint_to_string(unsigned int val) { return integral_to_string(val); }
		static auto long_to_string(signed long val) { return integral_to_string(val); }
		static auto ulong_to_string(unsigned long val) { return integral_to_string(val); }
		static auto longlong_to_string(signed long long val) { return integral_to_string(val); }
		static auto ulonglong_to_string(unsigned long long val) { return integral_to_string(val); }
	};

	template <class String>
	template <class SignedType>
	SignedType string_qvariant_converters<String>::string_to_signed(const string_type & str)
	{
		static_assert(std::is_signed_v<SignedType>);

		auto * first = str.data();
		auto * last = first + str.size();

		long long result = std::strtoll(first, &last, 10);
		constexpr long long min = std::numeric_limits<SignedType>::min();
		constexpr long long max = std::numeric_limits<SignedType>::max();
		return static_cast<SignedType>(std::clamp(min, result, max));
	}

	template <class String>
	template <class UnsignedType>
	UnsignedType string_qvariant_converters<String>::string_to_unsigned(const string_type & str)
	{
		static_assert(std::is_unsigned_v<UnsignedType>);

		auto * first = str.data();
		auto * last = first + str.size();

		unsigned long long result = std::strtoull(first, &last, 10);
		constexpr unsigned long long min = std::numeric_limits<UnsignedType>::min();
		constexpr unsigned long long max = std::numeric_limits<UnsignedType>::max();
		return static_cast<UnsignedType>(std::clamp(min, result, max));
	}

	template <class String>
	template <class IntegralType>
	auto string_qvariant_converters<String>::integral_to_string(IntegralType val) -> string_type
	{
		ext::itoa_buffer<IntegralType> buffer;
		return string_type(ext::itoa(val, buffer), std::end(buffer));
	}

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

		template <class StringType>
		double to_double(const StringType & str)
		{
			const char * first = str.data();
			return std::strtod(first, nullptr);
		}
	}

	template <class String>
	void register_string_metatype()
	{
		using string_type = String;

		qRegisterMetaType<string_type>();
		QMetaType::registerComparators<string_type>();

		QMetaType::registerConverter<string_type, QString>(static_cast<QString(*)(const string_type & )>(ToQString));
		QMetaType::registerConverter<QString, string_type>(static_cast<string_type(*)(const QString & )>(FromQString<string_type>));

		using register_helpers::to_integral;
		using register_helpers::from_integral;

        #define REGISTER_NUM_CONV(type) \
	        QMetaType::registerConverter<string_type, type>(static_cast<type (*)(const string_type &)>(to_integral<type, string_type>)); \
	        QMetaType::registerConverter<type, string_type>(static_cast<string_type(*)(type )>(from_integral<string_type, type>));

		REGISTER_NUM_CONV(signed short);
		REGISTER_NUM_CONV(unsigned short);
		REGISTER_NUM_CONV(signed int);
		REGISTER_NUM_CONV(unsigned int);
		REGISTER_NUM_CONV(signed long);
		REGISTER_NUM_CONV(unsigned long);
		REGISTER_NUM_CONV(signed long long);
		REGISTER_NUM_CONV(unsigned long long);
	}
}
