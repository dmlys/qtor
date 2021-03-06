﻿#pragma once
#include <string>
#include <vector>
#include <istream>
#include <regex>

#include <ext/itoa.hpp>
#include <ext/type_traits.hpp>
#include <ext/is_string.hpp>
#include <ext/range.hpp>
#include <boost/algorithm/string/find_format.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <fmt/format.h>

#include <qtor/torrent.hpp>
#include <qtor/torrent_file.hpp>


namespace qtor {
namespace transmission
{
	struct as_stdstring_t {} constexpr as_stdstring;
	struct as_ints_t {} constexpr as_ints;

	template <class Range, std::enable_if_t<ext::is_range_of<Range, QString>::value, int> = 0>
	inline auto operator |(const Range & range, as_stdstring_t) noexcept
	{
		return boost::adaptors::transform(range, [](const auto & str) { return FromQString(str); });
	}

	template <class Range, std::enable_if_t<ext::is_range_of<Range, std::string>::value, int> = 0>
	inline decltype(auto) operator |(const Range & range, as_stdstring_t) noexcept
	{
		return range;
	}
	
	template <class Range, std::enable_if_t<ext::is_range_of_v<Range, QString>, int> = 0>
	inline auto operator |(const Range & range, as_ints_t) noexcept
	{
		return boost::adaptors::transform(range, [](const QString & qstr) { return qstr.toLong(); });
	}

	template <class Range, std::enable_if_t<not ext::is_range_of_v<Range, QString>, int> = 0>
	inline auto operator |(const Range & range, as_ints_t) noexcept
	{
		return boost::adaptors::transform(range, [](const auto & str) { return std::stol(str); });
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
		extern const std::string command_template;
		extern const std::string command_template_all;

		extern const std::vector<std::string> request_default_fields;
		extern const std::vector<std::string> request_torrent_files_fields;
		extern const std::vector<std::string> request_torrent_peers_fields;
		extern const std::vector<std::string> request_trackers_fields;

		// commands
		extern const std::string torrent_get;
		extern const std::string torrent_set;
		extern const std::string torrent_add;

		extern const std::string torrent_start;
		extern const std::string torrent_start_now;
		extern const std::string torrent_stop;
		extern const std::string torrent_verify;
		extern const std::string torrent_reannounce;

		extern std::regex method_regex;
	}


	std::string_view extract_command(std::string_view command);

	
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
	
		auto sepr = ext::str_view(", ");
		auto quote = ext::str_view("\"");
		const auto & val = *first;
	
		ext::append(cont, begin(quote), end(quote));
		escape_json_string(ext::str_view(val), cont);
		ext::append(cont, begin(quote), end(quote));
	
		for (++first; first != last; ++first)
		{
			const auto & val = *first;
			ext::append(cont, begin(sepr), end(sepr));
			ext::append(cont, begin(quote), end(quote));
			escape_json_string(ext::str_view(val), cont);
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
	
		auto sepr = ext::str_view(", ");
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
	

	template <class IdsRange>
	std::string make_request_command(std::string_view command, const IdsRange & ids)
	{
		if (boost::empty(ids))
			return fmt::format(command_template_all, command);
		else
			return fmt::format(command_template, command, json_join(ids | as_ints));
	}

	template <class IdsRange, class FieldsRange>
	std::string make_request_command(std::string_view command, const IdsRange & ids, const FieldsRange & fields)
	{
		if (boost::empty(ids))
			return fmt::format(request_template_all, command, json_join(fields));
		else
			return fmt::format(request_template, command, json_join(fields), json_join(ids | as_ints));
	}

	template <class IdsRange, class FieldsRange>
	std::string make_torrent_get_command(const IdsRange & ids, const FieldsRange & fields)
	{
		return make_request_command(torrent_get, ids, fields);
	}

	template <class IdsRange>
	std::string make_torrent_get_command(const IdsRange & ids)
	{
		return make_torrent_get_command(ids, request_default_fields);
	}


	inline std::string make_torrent_files_get_command(const torrent_id_type & id)
	{
		auto ids = {id};
		return make_torrent_get_command(ids, request_torrent_files_fields);
	}

	inline std::string make_tracker_list_get_command(const torrent_id_type & id)
	{
		auto ids = {id};
		return make_torrent_get_command(ids, request_trackers_fields);
	}

	void parse_command_response(const std::string & json);
	void parse_command_response(std::istream & json_stream);

	torrent_list parse_torrent_list(const std::string & json);
	torrent_list parse_torrent_list(std::istream & json_stream);

	torrent_file_list parse_torrent_file_list(const std::string & json);
	torrent_file_list parse_torrent_file_list(std::istream & json_source);

	tracker_list parse_tracker_list(const std::string & json);
	tracker_list parse_tracker_list(std::istream & json_source);

	session_stat parse_statistics(const std::string & json);
	session_stat parse_statistics(std::istream & json_stream);
}}
