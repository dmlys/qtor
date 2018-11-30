#include <qtor/torrent_file.hpp>
#include <ext/config.hpp>

namespace qtor
{
	filepath_type get_name(const filepath_type & filepath)
	{
		int pos = filepath.lastIndexOf('/') + 1;
		return filepath.mid(pos);
	}

	/************************************************************************/
	/*                  torrent_file_meta                                   */
	/************************************************************************/
	const torrent_file_meta torrent_file_meta::instance;

	auto torrent_file_meta::item_count() const noexcept -> index_type
	{
		return FieldCount;
	}

	unsigned torrent_file_meta::item_type(index_type type) const noexcept
	{
		switch (type)
		{
			case FileName:
			case FilePath:  return model_meta::String;
			case TotalSize: return model_meta::Size;
			case HaveSize:  return model_meta::Size;
			case Index:     return model_meta::Int64;
			case Priority:  return model_meta::Int64;
			case Wanted:    return model_meta::Bool;

			default:
				EXT_UNREACHABLE();
		}
	}

	string_type torrent_file_meta::item_name(index_type item) const
	{
		switch (item)
		{
			case FileName:  return QStringLiteral("fname");
			case FilePath:  return QStringLiteral("fpath");
			case TotalSize: return QStringLiteral("total size");
			case HaveSize:  return QStringLiteral("have size");
			case Index:     return QStringLiteral("index");
			case Priority:  return QStringLiteral("priority");
			case Wanted:    return QStringLiteral("wanted");

			default:
				EXT_UNREACHABLE();
		}
	}

	bool torrent_file_meta::is_virtual_item(index_type index) const
	{
		return false;
	}

	auto torrent_file_meta::get_item(const torrent_file & item, index_type key) const -> any_type
	{
		switch (key)
		{
			case FileName:  return make_any(get_name(item.filename));
			case FilePath:  return make_any(item.filename);
			case TotalSize: return make_any(item.total_size);
			case HaveSize:  return make_any(item.have_size);
			case Index:     return make_any(item.index);
			case Priority:  return make_any(item.priority);
			case Wanted:    return make_any(item.wanted);

			default: return any_type();
		}
	}

	auto torrent_file_meta::get_item(const torrent_dir & item, index_type key) const -> any_type
	{
		switch (key)
		{
			case FileName:  return make_any(item.filename);
			case FilePath:  return make_any(item.filename);
			case TotalSize: return make_any(item.total_size);
			case HaveSize:  return make_any(item.have_size);
			case Index:     return make_any(item.index);
			case Priority:  return make_any(item.priority);
			case Wanted:    return make_any(item.wanted);

			default: return any_type();
		}
	}

	void torrent_file_meta::set_item(torrent_file & item, index_type key, const any_type & val) const
	{
		switch (key)
		{
			case FileName:  return;
			case FilePath:  item.filename   = qvariant_cast<filepath_type>(val); return;
			case TotalSize: item.total_size = qvariant_cast<size_type>(val); return;
			case HaveSize:  item.total_size = qvariant_cast<size_type>(val); return;
			case Index:     item.index      = qvariant_cast<int_type>(val); return;
			case Priority:  item.priority   = qvariant_cast<int_type>(val); return;
			case Wanted:    item.wanted     = qvariant_cast<bool_type>(val); return;

			default: return;
		}
	}

	void torrent_file_meta::set_item(torrent_dir & item, index_type key, const any_type & val) const
	{
		switch (key)
		{
			case FileName:  return;
			case FilePath:  item.filename   = qvariant_cast<filepath_type>(val); return;
			case TotalSize: item.total_size = qvariant_cast<size_type>(val); return;
			case HaveSize:  item.total_size = qvariant_cast<size_type>(val); return;
			case Index:     item.index      = qvariant_cast<int_type>(val); return;
			case Priority:  item.priority   = qvariant_cast<int_type>(val); return;
			case Wanted:    item.wanted     = qvariant_cast<Qt::CheckState>(val); return;

			default: return;
		}
	}
}
