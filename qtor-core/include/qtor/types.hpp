#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include <chrono>

#include <any>
#include <optional>
#include <variant>

//#include <boost/optional.hpp>
//#include <boost/variant.hpp>

#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtTools/ToolsBase.hpp>

namespace qtor
{
	inline namespace types
	{
		//typedef std::string    string_type;
		typedef QString        string_type;
		typedef std::uint64_t  uint64_type;
		typedef std::int64_t   int64_type;
		typedef int            int_type;
		typedef unsigned int   uint_type;
		typedef uint64_type    speed_type;
		typedef uint64_type    size_type;

		using datetime_type = std::chrono::system_clock::time_point;
		using duration_type = std::chrono::system_clock::duration;
		using filepath_type = QString;

		// using any, optional, variant classes from std
		template <class type>
		using optional = std::optional<type>;

		using nullopt_t = std::nullopt_t;
		constexpr nullopt_t nullopt = std::nullopt;
		using std::make_optional;
		using std::get;

		//using any = boost::any;
		//using boost::any_cast;
		//using bad_any_cast = boost::bad_any_cast;

		template <class ... types>
		using variant = std::variant<types...>;
	
		using any = QVariant;


		class bad_any_cast : public std::bad_cast
		{
		public:
			bad_any_cast() : std::bad_cast() {}

		public:
			virtual const char * what() const noexcept override { return "bad_any_cast"; }
		};

		template <class Type>
		inline any make_any(Type && val)
		{
			return QVariant::fromValue(std::forward<Type>(val));
		}

		template <class ValueType>
		ValueType * any_cast(QVariant * val)
		{
			if (not val or val->userType() != qMetaTypeId<ValueType>()) return nullptr;

			return reinterpret_cast<ValueType *>(val->data());
		}

		template <class ValueType>
		const ValueType * any_cast(const QVariant * val)
		{
			if (not val or val->userType() != qMetaTypeId<ValueType>()) return nullptr;

			return reinterpret_cast<const ValueType *>(val->data());
		}

		template <class ValueType>
		ValueType any_cast(const QVariant & val)
		{
			typedef std::remove_reference_t<ValueType> nonref;

			const nonref * result = any_cast<nonref>(&val);
			if (not result) throw std::bad_any_cast();

			return static_cast<ValueType>(*result);
		}

		template <class ValueType>
		ValueType any_cast(QVariant & val)
		{
			typedef std::remove_reference_t<ValueType> nonref;

			nonref * result = any_cast<nonref>(&val);
			if (not result) throw bad_any_cast();

			return static_cast<ValueType>(*result);
		}

		template <class ValueType>
		ValueType any_cast(QVariant && val)
		{
			typedef std::remove_reference_t<ValueType> nonref;

			nonref * result = any_cast<nonref>(&val);
			if (not result) throw std::bad_any_cast();

			return static_cast<ValueType>(std::move(*result));
		}
	}
}

//Q_DECLARE_METATYPE(qtor::speed_type)
//Q_DECLARE_METATYPE(qtor::size_type)
//Q_DECLARE_METATYPE(qtor::string_type);
Q_DECLARE_METATYPE(qtor::datetime_type);
Q_DECLARE_METATYPE(qtor::duration_type);
