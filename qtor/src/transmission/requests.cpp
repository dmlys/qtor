#include "requests.hpp"
#include <yaml-cpp/yaml.h>

namespace qtor {
namespace transmission
{
	inline namespace constants
	{
		const std::string torrent_get = "torrent-get";
		const std::string torrent_set = "torrent-set";
		const std::string torrent_add = "torrent-add";

		static const std::string Arguments = "arguments";
		static const std::string Torrents = "torrents";
		static const std::string Name = "name";
		static const std::string Id = "id";		
	}


	template <class Type>
	static void convert(const YAML::Node & node, Type & target)
	{
		target = node.as<Type>(target);
	}

	static torrent_list parse_torrent_list(const YAML::Node & node)
	{
		torrent_list result;

		const auto & res = node["result"].Scalar();
		if (res != "success")
			throw std::runtime_error(fmt::format("Bad response: " + res));

		YAML::Node arguments = node[Arguments][Torrents];
		for (YAML::Node tnode : arguments)
		{
			result.emplace_back();
			auto & torrent = result.back();

			convert(tnode[Id], torrent.id);
			convert(tnode[Name], torrent.name);
		}

		return result;
	}

	torrent_list parse_torrent_list(const std::string & json)
	{
		auto doc = YAML::Load(json);
		return parse_torrent_list(doc);
	}

	torrent_list parse_torrent_list(std::istream & json_stream)
	{
		auto doc = YAML::Load(json_stream);
		return parse_torrent_list(doc);
	}

}}
