#pragma once
#include <utility>
#include <functional>
#include <qtor/types.hpp>
#include <qtor/formatter.hqt>
#include <qtor/sparse_container.hpp>

#include <QtTools/ToolsBase.hpp>
#include <QtCore/QMetaType>

namespace qtor
{
	namespace torrent_status
	{		
		constexpr unsigned stopped = 0;
		constexpr unsigned download = 1;
		constexpr unsigned seed = 2;
		constexpr unsigned check = 3;
		constexpr unsigned check_wait = 4;
		constexpr unsigned download_wait = 5;
		constexpr unsigned seed_wait = 6;
	}


	constexpr bool REQ = true;
	constexpr bool OPT = false;


#define QTOR_TORRENT_FOR_EACH_BASIC_FIELD(F)              \
    /*opt, Id, Name, Type, TypeName */                    \
	F(REQ,   Id,      id,      string, torrent_id_type)   \
	F(REQ,   Name,    name,    string, string_type)       \
	F(OPT,   Creator, creator, string, string_type)       \
	F(OPT,   Comment, comment, string, string_type)       \


#define QTOR_TORRENT_FOR_EACH_STATUS_FEILD(F)                       \
	/*opt, Id, Name, Type, TypeName */                              \
	F(REQ, Status,      status,       uint64, uint64_type)          \
	F(OPT, ErrorString, error_string, string, string_type)          \
	F(OPT, Ratio,       ratio,        ratio,  double)               \
	F(OPT, SeedLimit,   seed_limit,   ratio,  double)               \
	                                                                \
	F(OPT, RequestedProgress, requested_progress,  percent, double) \
	F(OPT, TotalProgress,     total_progress,      percent, double) \
	F(OPT, RecheckProgress,   recheck_progress,    percent, double) \
	F(OPT, MetadataProgress,  metadata_progress,   percent, double) \
	                                                                \
	F(OPT, Finished,    finished,     bool,   bool)                 \
	F(OPT, Completed,   completed,    bool,   bool)                 \
	F(OPT, Stalled,     stalled,      bool,   bool)                 \


#define QTOR_TORRENT_FOR_EACH_SIZE_FIELD(F)                      \
	/* Id, Name, Type, TypeName */                               \
	F(OPT, CurrentSize,     current_size,      size, size_type)  \
	F(OPT, LeftSize,        left_size,         size, size_type)  \
	F(OPT, RequestedSize,   requested_size,    size, size_type)  \
	F(OPT, TotalSize,       total_size,        size, size_type)  \
	                                                             \
	F(OPT, EverUploaded,    ever_uploaded,     size, size_type)  \
	F(OPT, EverDownloaded,  ever_downloaded,   size, size_type)  \
	F(OPT, EverCurrupted,   ever_currupted,    size, size_type)  \



#define QTOR_TORRENT_FOR_EACH_SPEED_FIELD(F)                   \
	/* Id, Name, Type, TypeName */                             \
	F(OPT, DownloadSpeed, download_speed, speed, speed_type)   \
	F(OPT, UploadSpeed,   upload_speed,   speed, speed_type)   \


#define QTOR_TORRENT_FOR_EACH_TIME_DURATION_FIELD(F)         \
	/* Id, Name, Type, TypeName */                           \
	F(OPT, Eta,     eta,      duration, duration_type)       \
	F(OPT, EtaIdle, eta_idle, duration, duration_type)       \


#define QTOR_TORRENT_FOR_EACH_DATE_FIELD(F)                      \
	/* Id, Name, Type, TypeName */                               \
	F(OPT, DateAdded,   date_added,   datetime, datetime_type)   \
	F(OPT, DateCreated, date_created, datetime, datetime_type)   \
	F(OPT, DateStarted, date_started, datetime, datetime_type)   \
	F(OPT, DateDone,    date_done,    datetime, datetime_type)   \


#define QTOR_TORRENT_FOR_EACH_FIELD(F)                      \
	QTOR_TORRENT_FOR_EACH_BASIC_FIELD(F)                    \
	QTOR_TORRENT_FOR_EACH_STATUS_FEILD(F)                   \
	QTOR_TORRENT_FOR_EACH_SIZE_FIELD(F)                     \
	QTOR_TORRENT_FOR_EACH_SPEED_FIELD(F)                    \
	QTOR_TORRENT_FOR_EACH_TIME_DURATION_FIELD(F)            \
	QTOR_TORRENT_FOR_EACH_DATE_FIELD(F)                     \


#define QTOR_TORRENT_DEFINE_ENUM(AO, ID, NAME, A3, TYPE) ID,

#define QTOR_TORRENT_DEFINE_PROPERTY(AO, ID, NAME, A3, TYPE)                                                                   \
	auto NAME(TYPE val)        -> self_type &            { return static_cast<self_type &>(set_item(ID, std::move(val))); }    \
	auto NAME() const noexcept -> optional<const TYPE &> { return get_item<TYPE>(ID); }                                        \
	auto NAME() noexcept       -> optional<TYPE &>       { return get_item<TYPE>(ID); }                                        \
	                                                                                                                           \
	template <class Type>                                                                                                      \
	std::enable_if_t<std::is_convertible_v<std::decay_t<Type>, TYPE>, self_type &>                                             \
	NAME(optional<Type> val)                                                                                                   \
	{                                                                                                                          \
		return static_cast<self_type &>(set_item(ID, std::move(val)));                                                         \
	}                                                                                                                          \



	class torrent;
	struct torrent_file;
	struct torrent_peer;

	struct tracker_stat;
	struct session_stat;

	using torrent_id_type = string_type;
	using torrent_id_list = std::vector<torrent_id_type>;
	using torrent_list    = std::vector<torrent>;

	using torrent_peer_list = std::vector<torrent_peer>;
	using torrent_file_list = std::vector<torrent_file>;
	using tracker_list = std::vector<tracker_stat>;




	class torrent : public sparse_container
	{
		using self_type = torrent;
		using base_type = sparse_container;

	public:
		static const string_type ms_emptystr;

	public:
		enum : index_type
		{
			QTOR_TORRENT_FOR_EACH_FIELD(QTOR_TORRENT_DEFINE_ENUM)

			LastField,
			FirstField = 0,
		};

	public:
		QTOR_TORRENT_FOR_EACH_FIELD(QTOR_TORRENT_DEFINE_PROPERTY)
	};


	struct torrent_id_hasher
	{
		auto operator()(const torrent & val) const noexcept
		{
			return std::hash<torrent_id_type>{} (val.id().value_or(torrent::ms_emptystr));
		}
	};

	struct torrent_id_equal
	{
		bool operator()(const torrent & t1, const torrent & t2) const noexcept
		{
			return t1.id() == t2.id();
		}
	};

	template <class relation_comparator>
	struct torrent_id_comparator : relation_comparator
	{
		bool operator()(const torrent & t1, const torrent & t2) const noexcept
		{
			return relation_comparator::operator() (t1.id(), t2.id());
		}
	};

	typedef torrent_id_comparator<std::less<>>    torrent_id_less;
	typedef torrent_id_comparator<std::greater<>> torrent_id_greater;


	class torrent_meta : public simple_sparse_container_meta<>
	{
		using self_type = torrent_meta;
		using base_type = simple_sparse_container_meta;

	protected:
		static const item_map_ptr ms_items;

	public:
		torrent_meta(QObject * parent = nullptr);
	};
}
