#include <qtor/transmission/requests.hpp>
#include <ext/range/combine.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <QtCore/QJsonValue>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonParseError>
#include <QtTools/JsonTools.hpp>

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

	inline static bool valid(QJsonValue node)
	{
		return not node.isUndefined() and not node.isNull();
	}
	
	static void parse_string(const QJsonValue & node, torrent & t, torrent & (torrent::*pmf)(string_type val))
	{
		if (not valid(node)) return;

		auto data = node.toVariant();
		(t.*pmf)(data.toString());
	}

	static void parse_uint64(const QJsonValue & node, torrent & t, torrent & (torrent::*pmf)(uint64_type val))
	{		
		if (not valid(node)) return;

		auto data = node.toVariant();
		if (data.isValid() and data.canConvert<uint64_type>())
			(t.*pmf)(qvariant_cast<uint64_type>(data));
	}

	static void parse_double(const QJsonValue & node, torrent & t, torrent & (torrent::*pmf)(double val))
	{
		if (not valid(node)) return;

		double d = node.toDouble(NAN);
		if (not std::isnan(d)) (t.*pmf)(d);
	}

	static void parse_bool(const QJsonValue & node, torrent & t, torrent & (torrent::*pmf)(bool val))
	{
		if (not valid(node)) return;

		if (not node.isBool()) return;
		(t.*pmf)(node.toBool());
	}

	static void parse_speed(const QJsonValue & node, torrent & t, torrent & (torrent::*pmf)(speed_type val))
	{
		return parse_uint64(node, t, pmf);
	}

	static void parse_size(const QJsonValue & node, torrent & t, torrent & (torrent::*pmf)(size_type val))
	{
		return parse_uint64(node, t, pmf);
	}

	static void parse_datetime(const QJsonValue & node, torrent & t, torrent & (torrent::*pmf)(datetime_type val))
	{
		if (not valid(node)) return;

		auto data = node.toVariant();
		if (not data.isValid() or not data.canConvert<long long>()) return;

		long long seconds = qvariant_cast<long long>(data);
		if (seconds >= 0) (t.*pmf)(datetime_type::clock::from_time_t(seconds));
	}
	
	static void parse_duration(const QJsonValue & node, torrent & t, torrent & (torrent::*pmf)(duration_type val))
	{
		if (not valid(node)) return;

		auto data = node.toVariant();
		if (not data.isValid() or not data.canConvert<long long>()) return;

		long long seconds = qvariant_cast<long long>(data);
		if (seconds >= 0) (t.*pmf)(std::chrono::seconds(seconds));
	}

	static void parse_status(const QJsonValue & node, torrent & t)
	{
		auto status = node.toInt(-1);
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

	
	static torrent_list parse_torrent_list(const QJsonDocument & doc)
	{
		using QtTools::Json::get_path;
		using QtTools::Json::find_path;
		torrent_list result;

		auto root = doc.object();
		auto res = QtTools::FromQString(root["result"].toString());
		if (res != "success")
			throw std::runtime_error(fmt::format("Bad response: " + res));

		auto torrents = get_path(root, "arguments/torrents").toArray();
		for (const QJsonValue & tnode : torrents)
		{
			result.emplace_back();
			auto & torr = result.back();

			#define READ(type, name, where) parse_##type(find_path(tnode, name), torr, &torrent::where)

			parse_status(find_path(tnode, Status), torr);
			READ(string, Id, id);
			READ(string, Name, name);
			READ(string, Comment, comment);
			READ(string, Creator, creator);
			
			READ(string, ErrorString, error_string);
			READ(bool, IsFinished, finished);
			READ(bool, IsStalled, stalled);

			READ(size, LeftUntilDone, left_size);
			READ(size, SizeWhenDone, requested_size);
			READ(size, TotalSize, total_size);
			torr.current_size(torr.requested_size() - torr.left_size());

			READ(size, UploadedEver, ever_uploaded);
			READ(size, DownloadedEver, ever_downloaded);
			READ(size, CorruptEver, ever_currupted);

			if (auto ever_downloaded = torr.ever_downloaded())
				torr.ratio(torr.ever_uploaded() / ever_downloaded);
			else
				torr.ratio(torr.ever_uploaded() / torr.requested_size());

			READ(double, RecheckProgress, recheck_progress);
			READ(double, MetadataPercentComplete, metadata_progress);
			torr.requested_progress(torr.current_size() / torr.requested_size());
			torr.total_progress(torr.current_size() / torr.total_size());

			READ(duration, Eta, eta);
			READ(duration, EtaIdle, eta_idle);

			READ(speed, RateDownload, download_speed);
			READ(speed, RateUpload, upload_speed);

			READ(datetime, DateCreated, date_created);
			READ(datetime, AddedDate, date_added);
			READ(datetime, StartDate, date_started);
			READ(datetime, DoneDate, date_done);
			//READ(datetime, ActivityDate, date_last_activity);

			#undef READ
		}

		return result;
	}

	static torrent_file_list parse_torrent_file_list(const QJsonDocument & doc)
	{
		using QtTools::Json::get_path;
		torrent_file_list result;

		auto root = doc.object();
		auto res = QtTools::FromQString(root["result"].toString());
		if (res != "success")
			throw std::runtime_error(fmt::format("Bad response: " + res));

		auto files     = get_path(root, "arguments/torrents/0/files").toArray();
		auto fileStats = get_path(root, "arguments/torrents/0/fileStats").toArray();

		for (auto [file_node_ref, file_stat_node_ref] : ext::combine(files, fileStats))
		{
			QJsonValue file_node = file_node_ref;
			QJsonValue file_stat_node = file_stat_node_ref;
			torrent_file file;

			file.filename = file_node["name"].toString();
			file.total_size = file_node["length"].toDouble();
			file.have_size = file_node["bytesCompleted"].toDouble();
			file.wanted = file_stat_node["wanted"].toBool();
			file.priority = file_stat_node["priority"].toInt();

			result.push_back(std::move(file));
		}

		return result;
	}

	torrent_file_list parse_torrent_file_list(const std::string & json)
	{
		auto jdoc = QtTools::Json::parse_json(json);
		return parse_torrent_file_list(jdoc);
	}

	torrent_file_list parse_torrent_file_list(std::istream & json_source)
	{
		auto jdoc = QtTools::Json::parse_json(json_source);
		return parse_torrent_file_list(jdoc);
	}

	torrent_list parse_torrent_list(const std::string & json)
	{
		auto jdoc = QtTools::Json::parse_json(json);
		return parse_torrent_list(jdoc);
	}

	torrent_list parse_torrent_list(std::istream & json_stream)
	{
		auto jdoc = QtTools::Json::parse_json(json_stream);
		return parse_torrent_list(jdoc);
	}
}}
