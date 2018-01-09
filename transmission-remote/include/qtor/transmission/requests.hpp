#pragma once
#include <string>
#include <vector>
#include <istream>

#include <ext/type_traits.hpp>
#include <ext/is_string.hpp>
#include <ext/range.hpp>
#include <boost/algorithm/string/find_format.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <fmt/format.h>
#include <qtor/torrent.hpp>


namespace qtor {
namespace transmission
{
	struct as_stdstring_t {} const as_stdstring;

	template <class Range, std::enable_if_t<ext::is_range_of<Range, QString>::value, int> = 0>
	inline auto operator |(const Range & range, as_stdstring_t) noexcept
	{
		return boost::adaptors::transform(range, [](const auto & str) { return FromQString(str); });
	}

	template <class Range, std::enable_if_t<not ext::is_range_of<Range, QString>::value, int> = 0>
	inline decltype(auto) operator |(const Range & range, as_stdstring_t) noexcept
	{
		return range;
	}


	/// json request template.
	/// All requests follow same template:
	///   method - ...
	///   fields - ...
	///   ids    - ... for all field ids should be absent.
	inline namespace constants
	{
		extern const std::string request_template;
		extern const std::string request_template_all;
		extern const std::vector<std::string> request_default_fields;

		extern const std::string torrent_get;
		extern const std::string torrent_set;
		extern const std::string torrent_add;
		extern const std::string torrent_remove;
		extern const std::string torrent_set_location;
	}


	
	template <class String, class OutContainer>
	void escape_json_string(const String & val, OutContainer & cont)
	{
		using std::begin; using std::end;
		auto first = begin(val);
		auto last  = end(val);

		for (;;)
		{
			auto it = std::find_if(first, last, [](auto ch) { return ch == '"' or ch == '\\'; });
			ext::append(cont, first, it);

			if (it == last) break;
			
			cont.push_back('\\');
			cont.push_back(*it);

			first = ++it;
		}
	}

	template <class Range, class OutContainer>
	std::enable_if_t<ext::is_string<ext::range_value_t<Range>>::value>
	json_join(const Range & values, OutContainer & cont)
	{
		using std::begin; using std::end;
		auto first = begin(values);
		auto last  = end(values);
	
		if (first == last) return;
	
		auto sepr = ext::as_literal(", ");
		auto quote = ext::as_literal("\"");
		auto val = ext::as_literal(*first);
	
		ext::append(cont, begin(quote), end(quote));
		escape_json_string(val, cont);
		ext::append(cont, begin(quote), end(quote));
	
		for (++first; first != last; ++first)
		{
			val = ext::as_literal(*first);
			ext::append(cont, begin(sepr), end(sepr));
			ext::append(cont, begin(quote), end(quote));
			escape_json_string(val, cont);
			ext::append(cont, begin(quote), end(quote));
		}
	}
	
	template <class Range, class OutContainer>
	std::enable_if_t<std::is_arithmetic<ext::range_value_t<Range>>::value>
	json_join(const Range & values, OutContainer & cont)
	{
		typedef ext::range_value_t<Range> value_type;
		ext::itoa_buffer<value_type> buffer;
		char * bufend = buffer + sizeof(buffer) - 1;
	
		using std::begin; using std::end;
		auto first = begin(values);
		auto last  = end(values);
	
		if (first == last) return;
	
		auto sepr = ext::as_literal(", ");
		ext::append(cont, ext::itoa(*first, buffer), bufend);
	
		for (++first; first != last; ++first)
		{
			ext::append(cont, begin(sepr), end(sepr));
			ext::append(cont, ext::itoa(*first, buffer), bufend);
		}
	}
	
	template <class String = std::string, class Range>
	inline String json_join(const Range & values)
	{
		String str;
		json_join(values, str);
		return str;
	}
	

	template <class IdsRange, class FieldsRange>
	std::string make_request_command(const std::string & command, const IdsRange & ids, const FieldsRange & fields)
	{
		if (boost::empty(ids))
			return fmt::format(request_template_all, command, json_join(fields));
		else
			return fmt::format(request_template, command, json_join(fields), json_join(ids | as_stdstring));
	}

	template <class IdsRange>
	std::string make_request_command(const std::string & command, const IdsRange & ids)
	{
		return make_request_command(command, ids, request_default_fields);
	}


	torrent_list parse_torrent_list(const std::string & json);
	torrent_list parse_torrent_list(std::istream & json_stream);

	session_stat parse_statistics(const std::string & json);
	session_stat parse_statistics(std::istream & json_stream);
}}
