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
}
