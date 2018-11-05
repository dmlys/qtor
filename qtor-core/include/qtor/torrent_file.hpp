#pragma once
#include <qtor/types.hpp>
#include <qtor/model_meta.hpp>
#include <QtCore/QMetaType>

namespace qtor
{
	/************************************************************************/
	/*                   torrent_file                                       */
	/************************************************************************/

	struct torrent_file;
	struct torrent_dir;

	using torrent_file_list = std::vector<torrent_file>;
	using torrent_dir_list  = std::vector<torrent_dir>;

	// extracts file name from path in filepath and returns it
	filepath_type get_name(const filepath_type & filepath);

	struct torrent_file
	{
		filepath_type filename;
		size_type     total_size;
		size_type     have_size;
		int_type      index;
		int_type      priority;
		bool          wanted;
	};

	struct torrent_dir
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


	class torrent_file_meta : public model_meta
	{
	public:
		enum : unsigned
		{
			FileName,
			FilePath,
			TotalSize,
			HaveSize,
			Index,
			Priority,
			Wanted,

			FirstField = FileName,
			LastField = Wanted,
			FiledCount = LastField + 1,

		};

	public:
		virtual  index_type item_count()               const noexcept;
		virtual    unsigned item_type(index_type index) const noexcept;
		virtual string_type item_name(index_type index) const;

	public:
		any_type get_item(const torrent_file & item, int index) const;
		any_type get_item(const torrent_dir &  item, int index) const;
		any_type get_item(const torrent_file_entity & item, int index) const;

	public:
		torrent_file_meta() = default;
	};
}

Q_DECLARE_METATYPE(      qtor::torrent_file *)
Q_DECLARE_METATYPE(const qtor::torrent_file *)
Q_DECLARE_METATYPE(      qtor::torrent_dir  *)
Q_DECLARE_METATYPE(const qtor::torrent_dir  *)
Q_DECLARE_METATYPE(qtor::torrent_file_entity)
