#pragma once
#include <string>
#include <istream>

#include <ext/type_traits.hpp>
#include <ext/is_string.hpp>
#include <ext/range.hpp>
#include <boost/algorithm/string.hpp>

#include <fmt/format.h>
#include <torrent.hpp>

namespace qtor {
namespace transmission
{
	/// json request template.
	/// All requests follow same template:
	///   method - ...
	///   fields - ...
	///   ids    - ... for all field ids should be absent.
	inline namespace constants
	{
		const auto request_template = R"({{ "method": "{}", "arguments": {{ "fields": [ {} ], "ids": [ {} ] }} }})";
		const auto request_template_all = R"({{ "method": "{}", "arguments": {{ "fields": [ {} ] }} }})";

		const auto request_default_fields =
		{
			"id", "name"
		};

		extern const std::string torrent_get;
		extern const std::string torrent_set;
		extern const std::string torrent_add;
	}


	
	template <class String, class OutContainer>
	void escape_json_string(const String & val, OutContainer & cont)
	{
		using std::begin; using std::end;
		boost::replace_all_copy(std::back_inserter(cont), val, "\"", "\\\"");
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
			return fmt::format(request_template, command, json_join(fields), json_join(ids));
	}

	template <class IdsRange>
	std::string make_request_command(const std::string & command, const IdsRange & ids)
	{
		return make_request_command(command, ids, request_default_fields);
	}


	torrent_list parse_torrent_list(const std::string & json);
	torrent_list parse_torrent_list(std::istream & json_stream);

	statistics parse_statistics(const std::string & json);
	statistics parse_statistics(std::istream & json_stream);
}}
