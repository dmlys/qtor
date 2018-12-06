#include <qtor/transmission/requests.hpp>
#include <ext/range/combine.hpp>
#include <yaml-cpp/yaml.h>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <QtCore/QJsonValue>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonParseError>

namespace YAML
{
	template <>
	struct convert<QString>
	{
		static bool decode(const YAML::Node & node, QString & str)
		{
			str = ToQString(node.Scalar());
			return true;
		}
	};
}

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
		const std::string torrent_remove = "torrent-remove";
		const std::string torrent_set_location = "torrent-set-location";


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

		static const std::string PercentDone = "percentDone";
		static const std::string TotalProgress = "";
		static const std::string RecheckProgress = "recheckProgress";
		static const std::string MetadataPercentComplete = "metadataPercentComplete";		


		// size
		static const std::string TotalSize = "totalSize";
		static const std::string LeftUntilDone = "leftUntilDone";
		static const std::string SizeWhenDone = "sizeWhenDone";
		static const std::string UploadedEver = "uploadedEver";
		static const std::string DownloadedEver = "downloadedEver";
		static const std::string CorruptEver = "curruptEver";


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

		// files stats
		static const std::string Files = "files";
		static const std::string FileStats = "fileStats";

		// files parts
		static const std::string BytesCompleted = "bytesCompleted";
		static const std::string Length = "length";
		//static const std::string Name = "name";

		// fileStats parts
		static const std::string Wanted = "wanted";
		static const std::string Priority = "priority";

		// peers
		static const std::string Peers = "peers";

		// tracker stats
		static const std::string Trackers = "trackers";
		static const std::string TrackerStats = "trackerStats";



		//static const std::string BandwidthPriority = "bandwidthPriority";
		//static const std::string CorruptEver = "corruptEver";

		//static const std::string DesiredAvailable = "desiredAvailable";
		//static const std::string DownloadDir = "downloadDir";
		//static const std::string DownloadedEver = "downloadedEver";
		//static const std::string DownloadLimit = "downloadLimit";
		//static const std::string DownloadLimited = "downloadLimited";
		//static const std::string HaveUnchecked = "haveUnchecked";
		//static const std::string HaveValid = "haveValid";
		//static const std::string HonorsSessionLimits = "honorsSessionLimits";
		//static const std::string LeftUntilDone = "leftUntilDone";
		//static const std::string MagnetLink = "magnetLink";
		//static const std::string ManualAnnounceTime = "manualAnnounceTime";
		//static const std::string MaxConnectedPeers = "maxConnectedPeers";
		//static const std::string MetadataPercentComplete = "metadataPercentComplete";
		//static const std::string PeerLimit = "peer-limit";		
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
			LeftUntilDone, SizeWhenDone, TotalSize,
			DownloadedEver, UploadedEver, CorruptEver,
			RecheckProgress, MetadataPercentComplete,
			Eta, EtaIdle,
			RateDownload, RateUpload,
			AddedDate, DateCreated, StartDate, DoneDate, ActivityDate,
		};

		const std::vector<std::string> request_torrent_files_fields =
		{
			Files, FileStats,
		};

		const std::vector<std::string> request_torrent_peers_fields =
		{
			Trackers, TrackerStats,
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

	static void parse_status(const YAML::Node & node, torrent & t)
	{
		auto status = node.as<unsigned>(UINT_MAX);
		switch (status)
		{
			// as defined in transmission.h
			// TR_STATUS_STOPPED = 0, /* Torrent is stopped */
			case 0: status = torrent_status::stopped; break; 
			// TR_STATUS_CHECK_WAIT = 1, /* Queued to check files */
			case 1: status = torrent_status::checking_queued; break;
			// TR_STATUS_CHECK = 2, /* Checking files */
			case 2: status = torrent_status::checking; break;
			// TR_STATUS_DOWNLOAD_WAIT = 3, /* Queued to download */
			case 3: status = torrent_status::downloading_queued; break;
			// TR_STATUS_DOWNLOAD = 4, /* Downloading */
			case 4: status = torrent_status::downloading; break;
			// TR_STATUS_SEED_WAIT = 5, /* Queued to seed */
			case 5: status = torrent_status::seeding_queued; break;
			// TR_STATUS_SEED = 6  /* Seeding */
			case 6: status = torrent_status::seeding; break;
			
			default: status = torrent_status::unknown; break;
		}
		
		t.status(status);
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
	[[maybe_unused]] string_conv_type string_conv     = {&torr_val};                               \
	[[maybe_unused]] uint64_conv_type uint64_conv     = {&torr_val};                               \
	[[maybe_unused]] double_conv_type double_conv     = {&torr_val};                               \
	[[maybe_unused]] bool_conv_type bool_conv         = {&torr_val};                               \
	[[maybe_unused]] size_conv_type size_conv         = {&torr_val};                               \
	[[maybe_unused]] speed_conv_type speed_conv       = {&torr_val};                               \
	[[maybe_unused]] datetime_conv_type datetime_conv = {&torr_val};                               \
	[[maybe_unused]] duration_conv_type duration_conv = {&torr_val};                               \
	
	
	TYPE_CONV(string);
	TYPE_CONV(uint64);
	TYPE_CONV(double);
	TYPE_CONV(bool);
	TYPE_CONV(size);
	TYPE_CONV(speed);
	TYPE_CONV(datetime);
	TYPE_CONV(duration);


	template <class Type>
	static optional<std::decay_t<Type>> operator -(optional<Type> opt1, optional<Type> opt2)
	{
		if (not opt1 or not opt2) return nullopt;

		return opt1.value() - opt2.value();
	}

	template <class Type>
	static optional<double> operator /(optional<Type> opt1, optional<Type> opt2)
	{
		if (not opt1 or not opt2) return nullopt;

		return static_cast<double>(opt1.value()) / static_cast<double>(opt2.value());
	}
	
	static torrent_list parse_torrent_list(const YAML::Node & node)
	{
		torrent_list result;

		const auto & res = node["result"].Scalar();
		if (res != "success")
			throw std::runtime_error(fmt::format("Bad response: " + res));

		YAML::Node torrents = node[Arguments][Torrents];
		for (YAML::Node tnode : torrents)
		{
			result.emplace_back();
			auto & torr = result.back();

			DECLARE_CONVS(torr);

			parse_status(tnode[Status], torr);
			string_conv(tnode[Id], &torrent::id);
			string_conv(tnode[Name], &torrent::name);
			string_conv(tnode[Comment], &torrent::comment);
			string_conv(tnode[Creator], &torrent::creator);
			
			string_conv(tnode[ErrorString], &torrent::error_string);
			bool_conv(tnode[IsFinished], &torrent::finished);
			bool_conv(tnode[IsStalled], &torrent::stalled);

			size_conv(tnode[LeftUntilDone], &torrent::left_size);
			size_conv(tnode[SizeWhenDone], &torrent::requested_size);
			size_conv(tnode[TotalSize], &torrent::total_size);			
			torr.current_size(torr.requested_size() - torr.left_size());

			size_conv(tnode[UploadedEver], &torrent::ever_uploaded);
			size_conv(tnode[DownloadedEver], &torrent::ever_downloaded);
			size_conv(tnode[CorruptEver], &torrent::ever_currupted);

			torr.ratio(torr.ever_uploaded() / torr.ever_downloaded());

			double_conv(tnode[RecheckProgress], &torrent::recheck_progress);
			double_conv(tnode[MetadataPercentComplete], &torrent::metadata_progress);
			torr.requested_progress(torr.current_size() / torr.requested_size());
			torr.total_progress(torr.current_size() / torr.total_size());

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

	static torrent_file_list parse_torrent_file_list(const QJsonDocument & doc)
	{
		torrent_file_list result;

		auto root = doc.object();
		auto res = root["result"].toString();
		if (res != "success")
			throw std::runtime_error(fmt::format("Bad response: {}", res));

		QJsonObject data = root["arguments"].toObject()["torrents"].toArray()[0].toObject();
		QJsonArray files = data["files"].toArray();
		QJsonArray fileStats = data["fileStats"].toArray();

		for (auto [file_node, file_stat_node] : ext::combine(files, fileStats))
		{
			torrent_file file;

			file.filename = file_node.toObject()["name"].toString();
			file.total_size = file_node.toObject()["length"].toDouble();
			file.have_size = file_node.toObject()["bytesCompleted"].toDouble();
			file.wanted = file_stat_node.toObject()["wanted"].toBool();
			file.priority = file_stat_node.toObject()["priority"].toInt();

			result.push_back(std::move(file));
		}

		return result;
	}

	static torrent_file_list parse_torrent_file_list(const YAML::Node & node)
	{
		torrent_file_list result;

		const auto & res = node["result"].Scalar();
		if (res != "success")
			throw std::runtime_error(fmt::format("Bad response: " + res));

		YAML::Node files = node[Arguments][Torrents][0][Files];
		YAML::Node fileStats = node[Arguments][Torrents][0][FileStats];

		for (auto [file_node, file_stat_node] : ext::combine(files, fileStats))
		{
			torrent_file file;

			file.filename = file_node[Name].as<filepath_type>();
			file.total_size = file_node[Length].as<size_type>();
			file.have_size = file_node[BytesCompleted].as<size_type>();
			file.wanted = file_stat_node[Wanted].as<bool_type>();
			file.priority = file_stat_node[Priority].as<int_type>();

			result.push_back(std::move(file));
		}

		return result;
	}

	torrent_file_list parse_torrent_file_list(const std::string & json)
	{
		QByteArray source(json.c_str(), json.size());

		QJsonParseError err;
		QJsonDocument jdoc = jdoc.fromJson(source, &err);

		if (not jdoc.isNull()) return parse_torrent_file_list(jdoc);

		//auto doc = YAML::Load(json);
		//return parse_torrent_file_list(doc);
	}

	torrent_file_list parse_torrent_file_list(std::istream & json_source)
	{
		auto doc = YAML::Load(json_source);
		return parse_torrent_file_list(doc);
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
