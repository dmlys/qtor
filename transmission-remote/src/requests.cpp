#include <qtor/transmission/requests.hpp>
#include <yaml-cpp/yaml.h>

namespace qtor {
namespace transmission
{
	// for simplicity of macros
	using double_type = double;
	using bool_type   = bool;

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

		// speeds
		static const std::string RateDownload = "rateDownload";
		static const std::string RateUpload = "rateUpload";

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
			RateDownload, RateUpload,
			AddedDate, DateCreated, StartDate, DoneDate, ActivityDate,
		};
	}

	
	static void parse_string(const YAML::Node & node, torrent & t, torrent & (torrent::*pmf)(string_type val))
	{
		if (not node) return;

		(t.*pmf)(node.as<string_type>());
	}

	static void parse_uint64(const YAML::Node & node, torrent & t, torrent & (torrent::*pmf)(uint64_type val))
	{		
		if (not node) return;

		long long i = node.as<long long>(-1);
		if (i >= 0) (t.*pmf)(static_cast<uint64_type>(i));
	}

	static void parse_double(const YAML::Node & node, torrent & t, torrent & (torrent::*pmf)(double val))
	{
		if (not node) return;

		double d = node.as<double>(NAN);
		if (not std::isnan(d)) (t.*pmf)(d);
	}

	static void parse_bool(const YAML::Node & node, torrent & t, torrent & (torrent::*pmf)(bool val))
	{
		if (not node) return;

		int i = 2;
		bool def = reinterpret_cast<bool &>(i);
		bool b = node.as<bool>(def);
		if (b != def) (t.*pmf)(b);
	}

	static void parse_speed(const YAML::Node & node, torrent & t, torrent & (torrent::*pmf)(speed_type val))
	{
		return parse_uint64(node, t, pmf);
	}

	static void parse_size(const YAML::Node & node, torrent & t, torrent & (torrent::*pmf)(size_type val))
	{
		return parse_uint64(node, t, pmf);
	}

	static void parse_datetime(const YAML::Node & node, torrent & t, torrent & (torrent::*pmf)(datetime_type val))
	{
		if (not node) return;

		long long seconds = node.as<long long>(-1);
		if (seconds >= 0) (t.*pmf)(datetime_type::clock::from_time_t(seconds));
	}
	
	static void parse_duration(const YAML::Node & node, torrent & t, torrent & (torrent::*pmf)(duration_type val))
	{
		if (not node) return;

		long long seconds = node.as<long long>(-1);
		if (seconds >= 0) (t.*pmf)(std::chrono::seconds(seconds));
	}

	
	
#define TYPE_CONV(TYPE)                                                                            \
	struct TYPE##_conv_type                                                                        \
	{                                                                                              \
		torrent * torr;                                                                            \
		                                                                                           \
		void operator()(const YAML::Node & node, torrent & (torrent::*pmf)(TYPE##_type val)) const \
		{                                                                                          \
			parse_##TYPE(node, *torr, pmf);                                                        \
		}                                                                                          \
	};

#define DECLARE_CONVS(torr_val)                                                                    \
	string_conv_type string_conv     = {&torr_val};                                                \
	uint64_conv_type uint64_conv     = {&torr_val};                                                \
	double_conv_type double_conv     = {&torr_val};                                                \
	bool_conv_type bool_conv         = {&torr_val};                                                \
	size_conv_type size_conv         = {&torr_val};                                                \
	speed_conv_type speed_conv       = {&torr_val};                                                \
	datetime_conv_type datetime_conv = {&torr_val};                                                \
	duration_conv_type duration_conv = {&torr_val};                                                \
	
	
	TYPE_CONV(string);
	TYPE_CONV(uint64);
	TYPE_CONV(double);
	TYPE_CONV(bool);
	TYPE_CONV(size);
	TYPE_CONV(speed);
	TYPE_CONV(datetime);
	TYPE_CONV(duration);


	
	
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

			DECLARE_CONVS(torr);
			
			string_conv(tnode[Id], &torrent::id);
			string_conv(tnode[Name], &torrent::name);
			string_conv(tnode[Comment], &torrent::comment);
			string_conv(tnode[Creator], &torrent::creator);

			string_conv(tnode[ErrorString], &torrent::error_string);
			bool_conv(tnode[IsFinished], &torrent::finished);
			bool_conv(tnode[IsStalled], &torrent::stalled);

			size_conv(tnode[TotalSize], &torrent::total_size);
			size_conv(tnode[SizeWhenDone], &torrent::size_when_done);

			duration_conv(tnode[Eta], &torrent::eta);
			duration_conv(tnode[EtaIdle], &torrent::eta_idle);
			
			speed_conv(tnode[RateDownload], &torrent::download_speed);
			speed_conv(tnode[RateUpload], &torrent::upload_speed);

			datetime_conv(tnode[DateCreated], &torrent::date_created);
			datetime_conv(tnode[AddedDate], &torrent::date_added);
			datetime_conv(tnode[StartDate], &torrent::date_started);
			datetime_conv(tnode[DoneDate], &torrent::date_done);
			//datetime_conv(tnode[ActivityDate], &torrent::date_last_activity);
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
