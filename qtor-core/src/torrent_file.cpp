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
	auto torrent_file_meta::item_count() const noexcept -> index_type
	{
		return FiledCount;
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

	auto torrent_file_meta::get_item(const torrent_file & item, index_type index) const -> any_type
	{
		switch (index)
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

	auto torrent_file_meta::get_item(const torrent_dir & item, index_type index) const -> any_type
	{
		switch (index)
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
}
