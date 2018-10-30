#pragma once
#include <utility>
#include <functional>
#include <boost/preprocessor/if.hpp>

#include <qtor/types.hpp>
#include <qtor/formatter.hqt>
#include <qtor/sparse_container.hpp>

#include <QtTools/ToolsBase.hpp>

namespace qtor
{
	namespace torrent_status
	{		
		constexpr unsigned unknown = -1;
		constexpr unsigned stopped = 0;
		constexpr unsigned checking = 1;
		constexpr unsigned checking_queued = 2;
		constexpr unsigned downloading = 3;
		constexpr unsigned downloading_queued = 4;
		constexpr unsigned seeding = 5;
		constexpr unsigned seeding_queued = 6;
	}

	class  torrent;
	struct torrent_file;
	struct torrent_peer;

	struct tracker_stat;
	struct session_stat;

	using torrent_id_type = string_type;
	using torrent_id_list = std::vector<torrent_id_type>;
	using torrent_list = std::vector<torrent>;

	using torrent_peer_list = std::vector<torrent_peer>;
	using torrent_file_list = std::vector<torrent_file>;
	using tracker_list = std::vector<tracker_stat>;



	/************************************************************************/
	/*                   torrent_file                                       */
	/************************************************************************/
	struct torrent_file_enum
	{
		enum : unsigned
		{
			FileName,
			TotalSize,
			HaveSize,
			Index,
			Priority,
			Wanted,

			FirstField = FileName,
			LastField = Wanted,
			FiledCount = LastField + 1,
		};
	};

	struct torrent_file : torrent_file_enum
	{
		filepath_type filename;
		size_type     total_size;
		size_type     have_size;
		int_type      index;
		int_type      priority;
		bool          wanted;
	};

	struct torrent_dir : torrent_file_enum
	{
		filepath_type filename;
		size_type     total_size;
		size_type     have_size;
		int_type      index;
		int_type      priority;
		Qt::CheckState wanted;
	};


	using torrent_file_entity =	variant
	<
		const torrent_file *,
		const torrent_dir *
	>;



	struct torrent_file_id_hasher
	{
		auto operator()(const torrent_file & val) const noexcept
		{
			return std::hash<filepath_type>{} (val.filename);
		}
	};

	struct torrent_file_id_equal
	{
		bool operator()(const torrent_file & f1, const torrent_file & f2) const noexcept
		{
			return f1.filename == f2.filename;
		}
	};

	template <class relation_comparator>
	struct torrent_file_id_comparator : relation_comparator
	{
		bool operator()(const torrent_file & f1, const torrent_file & f2) const noexcept
		{
			return relation_comparator::operator()(f1.filename, f2.filename);
		}
	};

	using torrent_file_id_less    = torrent_file_id_comparator<std::less<>>;
	using torrent_file_id_greater = torrent_file_id_comparator<std::greater<>>;


	class torrent_file_meta :
		public virtual model_meta,
		public virtual formatter
	{
	public:
		using formatter::format_item;

	public:
		virtual  index_type item_count()               const noexcept;
		virtual    unsigned item_type(index_type type) const noexcept;
		virtual string_type item_name(index_type type) const;

		virtual  QString format_item(const torrent_file & val, index_type index) const;
		virtual  QString format_item(const torrent_dir & val, index_type index) const;
		virtual  QString format_item_short(const torrent_file & val, index_type index) const;
		virtual  QString format_item_short(const torrent_dir & val, index_type index) const;

		QString format_item(const torrent_file_entity & val, index_type index) const;
		QString format_item_short(const torrent_file_entity & val, index_type index) const;

	public:
		torrent_file_meta(QObject * parent = nullptr);
	};

	struct torrent_peer
	{
		bool client_choked;
		bool client_interested;
		bool downloading_from;
		bool uploading_to;
		bool encrypted;
		bool incoming;		
		bool peer_choked;
		bool peer_interested;
		string_type address;
		string_type client_name;
		string_type flag_str;
		int port;
		speed_type rate_to_client;
		speed_type rate_to_peer;
		double progress;
	};


	struct session_stat
	{

	};


	/************************************************************************/
	/*                       torrent                                        */
	/************************************************************************/	

#define QTOR_REQ 1
#define QTOR_OPT 0


#define QTOR_TORRENT_FOR_EACH_BASIC_FIELD(F)                                    \
    /*opt/req, Id, Name, Type, TypeName */                                      \
	F(QTOR_REQ,   Id,      id,      string, torrent_id_type)                    \
	F(QTOR_REQ,   Name,    name,    string, string_type)                        \
	F(QTOR_OPT,   Creator, creator, string, string_type)                        \
	F(QTOR_OPT,   Comment, comment, string, string_type)                        \


#define QTOR_TORRENT_FOR_EACH_STATUS_FEILD(F)                                   \
	/*opt, Id, Name, Type, TypeName */                                          \
	F(QTOR_REQ, Status,      status,       uint64, uint64_type)                 \
	F(QTOR_OPT, ErrorString, error_string, string, string_type)                 \
	                                                                            \
	F(QTOR_OPT, Finished,    finished,     bool,   bool)                        \
	F(QTOR_OPT, Completed,   completed,    bool,   bool)                        \
	F(QTOR_OPT, Stalled,     stalled,      bool,   bool)                        \
	                                                                            \
	F(QTOR_OPT, UploadingPeers,   uploading_peers,   uint64, uint64_type)       \
	F(QTOR_OPT, DownloadingPeers, downloading_peers, uint64, uint64_type)       \
	F(QTOR_OPT, ConnectedPeers,   connected_peers,   uint64, uint64_type)       \
	                                                                            \
	F(QTOR_OPT, DownloadingWebseeds, downloading_webseeds, uint64, uint64_type) \
	F(QTOR_OPT, ConnectedWebseeds,   connected_webseeds,   uint64, uint64_type) \



#define QTOR_TORRENT_FOR_EACH_PROGRESS_FIELD(F)                          \
	F(QTOR_OPT, Ratio,       ratio,        ratio,  double)               \
	F(QTOR_OPT, SeedLimit,   seed_limit,   ratio,  double)               \
	                                                                     \
	F(QTOR_OPT, RequestedProgress, requested_progress,  percent, double) \
	F(QTOR_OPT, TotalProgress,     total_progress,      percent, double) \
	F(QTOR_OPT, RecheckProgress,   recheck_progress,    percent, double) \
	F(QTOR_OPT, MetadataProgress,  metadata_progress,   percent, double) \




#define QTOR_TORRENT_FOR_EACH_SIZE_FIELD(F)                           \
	/* Id, Name, Type, TypeName */                                    \
	F(QTOR_OPT, CurrentSize,     current_size,      size, size_type)  \
	F(QTOR_OPT, LeftSize,        left_size,         size, size_type)  \
	F(QTOR_OPT, RequestedSize,   requested_size,    size, size_type)  \
	F(QTOR_OPT, TotalSize,       total_size,        size, size_type)  \
	                                                                  \
	F(QTOR_OPT, EverUploaded,    ever_uploaded,     size, size_type)  \
	F(QTOR_OPT, EverDownloaded,  ever_downloaded,   size, size_type)  \
	F(QTOR_OPT, EverCurrupted,   ever_currupted,    size, size_type)  \



#define QTOR_TORRENT_FOR_EACH_SPEED_FIELD(F)                        \
	/* Id, Name, Type, TypeName */                                  \
	F(QTOR_OPT, DownloadSpeed, download_speed, speed, speed_type)   \
	F(QTOR_OPT, UploadSpeed,   upload_speed,   speed, speed_type)   \


#define QTOR_TORRENT_FOR_EACH_TIME_DURATION_FIELD(F)                  \
	/* Id, Name, Type, TypeName */                                    \
	F(QTOR_OPT, Eta,     eta,      duration, duration_type)           \
	F(QTOR_OPT, EtaIdle, eta_idle, duration, duration_type)           \


#define QTOR_TORRENT_FOR_EACH_DATE_FIELD(F)                           \
	/* Id, Name, Type, TypeName */                                    \
	F(QTOR_OPT, DateAdded,   date_added,   datetime, datetime_type)   \
	F(QTOR_OPT, DateCreated, date_created, datetime, datetime_type)   \
	F(QTOR_OPT, DateStarted, date_started, datetime, datetime_type)   \
	F(QTOR_OPT, DateDone,    date_done,    datetime, datetime_type)   \


#define QTOR_TORRENT_FOR_EACH_FIELD(F)                      \
	QTOR_TORRENT_FOR_EACH_BASIC_FIELD(F)                    \
	QTOR_TORRENT_FOR_EACH_STATUS_FEILD(F)                   \
	QTOR_TORRENT_FOR_EACH_PROGRESS_FIELD(F)                 \
	QTOR_TORRENT_FOR_EACH_SIZE_FIELD(F)                     \
	QTOR_TORRENT_FOR_EACH_SPEED_FIELD(F)                    \
	QTOR_TORRENT_FOR_EACH_TIME_DURATION_FIELD(F)            \
	QTOR_TORRENT_FOR_EACH_DATE_FIELD(F)                     \


#define QTOR_TORRENT_DEFINE_ENUM(AO, ID, NAME, A3, TYPE) ID,


#define QTOR_TORRENT_DEFINE_PROPERTY(OPT_REQ, ID, NAME, A3, TYPE) \
	 BOOST_PP_IF(OPT_REQ, QTOR_TORRENT_DEFINE_REQ_PROPERTY, QTOR_TORRENT_DEFINE_OPT_PROPERTY)(ID, NAME, A3, TYPE)

#define QTOR_TORRENT_DEFINE_REQ_PROPERTY(ID, NAME, A3, TYPE)                                                     \
	auto NAME(TYPE val) -> self_type &     { return static_cast<self_type &>(set_item(ID, std::move(val))); }    \
	auto NAME() const   -> TYPE            { return get_item<TYPE>(ID).value(); }                                \
	                                                                                                             \
	/*template <class Type>                                                                                        \
	std::enable_if_t<std::is_convertible_v<std::decay_t<Type>, TYPE>, self_type &>                               \
	NAME(optional<Type> val)                                                                                     \
	{                                                                                                            \
	    return static_cast<self_type &>(set_item(ID, std::move(val)));                                           \
	}*/                                                                                                            \


#define QTOR_TORRENT_DEFINE_OPT_PROPERTY(ID, NAME, A3, TYPE)                                                     \
	auto NAME(TYPE val) -> self_type &     { return static_cast<self_type &>(set_item(ID, std::move(val))); }    \
	auto NAME() const   -> optional<TYPE>  { return get_item<TYPE>(ID); }                                        \
	/*auto NAME()         -> optional<TYPE &>       { return get_item<TYPE>(ID); }  */                           \
	                                                                                                             \
	template <class Type>                                                                                        \
	std::enable_if_t<std::is_convertible_v<std::decay_t<Type>, TYPE>, self_type &>                               \
	NAME(optional<Type> val)                                                                                     \
	{                                                                                                            \
		return static_cast<self_type &>(set_item(ID, std::move(val)));                                           \
	}                                                                                                            \



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
			return std::hash<torrent_id_type>{} (val.id());
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

	using torrent_id_less    = torrent_id_comparator<std::less<>>;
	using torrent_id_greater = torrent_id_comparator<std::greater<>>;


	class torrent_meta : public simple_sparse_container_meta
	{
		using self_type = torrent_meta;
		using base_type = simple_sparse_container_meta;

	protected:
		static const item_map_ptr ms_items;

	public:
		torrent_meta(QObject * parent = nullptr);
	};
}
