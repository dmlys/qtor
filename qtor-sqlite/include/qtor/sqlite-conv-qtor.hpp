#pragma once
#include <qtor/types.hpp>
#include <sqlite3yaw/convert.hpp>

namespace sqlite3yaw::convert
{
	template <>
	struct conv<qtor::datetime_type>
	{
		static void put(qtor::datetime_type val, bool temp, ibind & b)
		{
			auto secs = qtor::datetime_type::clock::to_time_t(val);
			conv<decltype(secs)>::put(secs, false, b);
		}

		static void get(qtor::datetime_type & val, iquery & q)
		{
			val = qtor::datetime_type::clock::from_time_t(q.get_int64());
		}
	};

	template <>
	struct conv<qtor::duration_type>
	{
		static void put(qtor::duration_type val, bool, ibind & b)
		{
			using namespace std::chrono;
			auto secs = duration_cast<seconds>(val).count();
			conv<decltype(secs)>::put(secs, false, b);
		}

		static void get(qtor::duration_type & val, iquery & q)
		{
			val = std::chrono::seconds(q.get_int64());
		}
	};

	template <>
	struct conv<bool>
	{
		static void put(bool val, bool temp, ibind & b)
		{
			conv<int>::put(val, false, b);
		}

		static void get(bool & val, iquery & q)
		{
			val = q.get_int();
		}
	};

	template <>
	struct conv<std::nullopt_t>
	{
		static void put(std::nullopt_t val, bool temp, ibind & b)
		{
			conv<std::nullptr_t>::put(nullptr, false, b);
		}
	};

	template <class Type>
	struct conv<std::optional<Type>>
	{
		typedef std::optional<Type> optional;

		static void put(const optional & val, bool temp, ibind & b)
		{
			if (val)
				conv<Type>::put(val.value(), temp, b);
			else
				conv<std::nullptr_t>::put(nullptr, false, b);
		}

		static void get(optional & val, iquery & q)
		{
			if (q.get_type() == SQLITE_NULL)
				val = nullopt;
			else
			{
				Type v;
				conv<Type>::get(q, v);
				val = std::move(v);
			}
		}
	};


	template <class ... Type>
	struct conv<std::variant<Type...>>
	{
		typedef std::variant<Type...> variant;

		static void put(const variant & val, bool temp, ibind & b)
		{
			auto vis = [&b, temp](const auto & val) 
			{
				conv<std::decay_t<decltype(val)>>::put(val, temp, b);
			};

			std::visit(vis, val);
		}
	};

}
