#include <qtor/transmission/requests.hpp>
#include <yaml-cpp/yaml.h>

namespace qtor {
namespace transmission
{
	inline namespace constants
	{
		const std::string request_template = R"({{ "method": "{}", "arguments": {{ "fields": [ {} ], "ids": [ {} ] }} }})";
		const std::string request_template_all = R"({{ "method": "{}", "arguments": {{ "fields": [ {} ] }} }})";

		// commands
		const std::string torrent_get = "torrent-get";
		const std::string torrent_set = "torrent-set";
		const std::string torrent_add = "torrent-add";

		// fields helpers
		static const std::string Arguments = "arguments";
		static const std::string Torrents = "torrents";

		// fields
		// basic
		static const std::string Id = "id";
		static const std::string Name = "name";
		static const std::string Comment = "comment";
		static const std::string Creator = "creator";
		static const std::string HashString = "hashString";

		// status
		static const std::string Status = "status";
		static const std::string Error = "error";
		static const std::string ErrorString = "errorString";

		static const std::string IsFinished = "isFinished";
		static const std::string IsPrivate = "isPrivate";
		static const std::string IsStalled = "isStalled";

		// size
		static const std::string TotalSize = "totalSize";
		static const std::string SizeWhenDone = "sizeWhenDone";

		// times
		static const std::string Eta = "eta";
		static const std::string EtaIdle = "etaIdle";

		// dates
		static const std::string AddedDate = "addedDate";
		static const std::string DateCreated = "dateCreated";
		static const std::string StartDate = "startDate";
		static const std::string DoneDate = "doneDate";
		static const std::string ActivityDate = "activityDate";



		//static const std::string BandwidthPriority = "bandwidthPriority";
		//static const std::string CorruptEver = "corruptEver";

		//static const std::string DesiredAvailable = "desiredAvailable";
		//static const std::string DownloadDir = "downloadDir";
		//static const std::string DownloadedEver = "downloadedEver";
		//static const std::string DownloadLimit = "downloadLimit";
		//static const std::string DownloadLimited = "downloadLimited";
		//static const std::string Files = "files";
		//static const std::string FileStats = "fileStats";
		//static const std::string HaveUnchecked = "haveUnchecked";
		//static const std::string HaveValid = "haveValid";
		//static const std::string HonorsSessionLimits = "honorsSessionLimits";
		//static const std::string LeftUntilDone = "leftUntilDone";
		//static const std::string MagnetLink = "magnetLink";
		//static const std::string ManualAnnounceTime = "manualAnnounceTime";
		//static const std::string MaxConnectedPeers = "maxConnectedPeers";
		//static const std::string MetadataPercentComplete = "metadataPercentComplete";
		//static const std::string PeerLimit = "peer-limit";
		//static const std::string Peers = "peers";
		//static const std::string PeersConnected = "peersConnected";
		//static const std::string PeersFrom = "peersFrom";
		//static const std::string PeersGettingFromUs = "peersGettingFromUs";
		//static const std::string PeersSendingToUs = "peersSendingToUs";
		//static const std::string PercentDone = "percentDone";
		//static const std::string Pieces = "pieces";
		//static const std::string PieceCount = "pieceCount";
		//static const std::string PieceSize = "pieceSize";
		//static const std::string Priorities = "priorities";
		//static const std::string QueuePosition = "queuePosition";
		//static const std::string RateDownload = "rateDownload";
		//static const std::string RateUpload = "rateUpload";
		//static const std::string RecheckProgress = "recheckProgress";
		//static const std::string SecondsDownloading = "secondsDownloading";
		//static const std::string SecondsSeeding = "secondsSeeding";
		//static const std::string SeedIdleLimit = "seedIdleLimit";
		//static const std::string SeedIdleMode = "seedIdleMode";
		//static const std::string SeedRatioLimit = "seedRatioLimit";
		//static const std::string SeedRatioMode = "seedRatioMode";
		//static const std::string Trackers = "trackers";
		//static const std::string TrackerStats = "trackerStats";
		//static const std::string TorrentFile = "torrentFile";
		//static const std::string UploadedEver = "uploadedEver";
		//static const std::string UploadLimit = "uploadLimit";
		//static const std::string UploadLimited = "uploadLimited";
		//static const std::string UploadRatio = "uploadRatio";
		//static const std::string Wanted = "wanted";
		//static const std::string Webseeds = "webseeds";
		//static const std::string WebseedsSendingToUs = "webseedsSendingToUs";


		const std::vector<std::string> request_default_fields =
		{
			Id, Name, Comment, Creator,
			Status, Error, ErrorString, IsFinished, IsStalled,
			TotalSize, SizeWhenDone,
			Eta, EtaIdle,
			AddedDate, DateCreated, StartDate, DoneDate, ActivityDate,
		};
	}


	template <class Type>
	static void convert(const YAML::Node & node, Type & target)
	{
		target = node.as<Type>(target);
	}

	template <class Type>
	static void convert(const YAML::Node & node, torrent & t, torrent & (torrent::*pmf)(Type val))
	{
		Type val;
		convert(node, val);

		(t.*pmf)(val);
	}

	struct conv_type
	{
		torrent * torr;

		template <class Type>
		void operator()(const YAML::Node & node, torrent & (torrent::*pmf)(Type val))
		{
			convert(node, *torr, pmf);
		}
	};

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
			auto & torr = result.back();

			conv_type conv = {&torr};
			conv(tnode[Id], &torrent::id);
			conv(tnode[Name], &torrent::name);
			conv(tnode[Comment], &torrent::comment);
			conv(tnode[Creator], &torrent::creator);

			conv(tnode[ErrorString], &torrent::error_string);
			conv(tnode[IsFinished], &torrent::finished);
			conv(tnode[IsStalled], &torrent::stalled);

			conv(tnode[TotalSize], &torrent::total_size);
			conv(tnode[SizeWhenDone], &torrent::size_when_done);

			//conv(tnode[Eta], &torrent::eta);
			//conv(tnode[EtaIdle], &torrent::eta_idle);
			
			//conv(tnode[DowloadSpeed], &torrent::download_speed);
			//conv(tnode[UploadSpeed], &torrent::upload_speed);

			//conv(tnode[DateCreated], &torrent::date_created);
			//conv(tnode[AddedDate], &torrent::date_added);
			//conv(tnode[StartDate], &torrent::date_started);
			//conv(tnode[DoneDate], &torrent::date_done);
			//conv(tnode[ActivityDate], &torrent::date_last_activity);
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

//namespace YAML
//{
//	template <>
//	struct convert<qtor::datetime_type>
//	{
//		static bool decode(const Node & node, qtor::datetime_type & rhs)
//		{
//			if (!node.IsScalar())
//				return false;
//			auto val = node.Scalar();
//			rhs.assign(val.data(), val.length());
//			return true;
//		}
//
//		static Node encode(const qtor::datetime_type & rhs)
//		{
//			Node node;
//			node = std::string(rhs.data(), rhs.length());
//			return node;
//		}
//	};
//
//	template <>
//	struct convert<qtor::duration_type>
//	{
//		static bool decode(const Node & node, qtor::duration_type & rhs)
//		{
//			if (!node.IsScalar())
//				return false;
//			auto val = node.Scalar();
//			rhs.assign(val.data(), val.length());
//			return true;
//		}
//
//		static Node encode(const qtor::duration_type & rhs)
//		{
//			Node node;
//			node = std::string(rhs.data(), rhs.length());
//			return node;
//		}
//	};
//}
