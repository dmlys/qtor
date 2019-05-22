#include <qtor/TorrentCategoryModel.hqt>
#include <QtTools/Utility.hpp>
#include <QtCore/QLocale>

namespace qtor
{
	QString TorrentCategoryModel::NameForCategory(category_type cat) const
	{
		switch (cat)
		{
			case category_type::all:             return tr("all");
			case category_type::downloading:     return tr("downloading");
			case category_type::downloaded:      return tr("downloaded");
			case category_type::active:          return tr("active");
			case category_type::nonactive:       return tr("nonactive");
			case category_type::stopped:         return tr("stopped");
			case category_type::error:           return tr("error");
			default:                             return tr("Unknown category");
		}
	}

	QIcon TorrentCategoryModel::IconForCategory(category_type cat) const
	{
		switch (cat)
		{
			case category_type::all:             return QIcon::fromTheme("");
			case category_type::downloading:     return QIcon::fromTheme("");
			case category_type::downloaded:      return QIcon::fromTheme("");
			case category_type::active:          return QIcon::fromTheme("");
			case category_type::nonactive:       return QIcon::fromTheme("");
			case category_type::stopped:         return QIcon::fromTheme("");
			case category_type::error:           return QIcon::fromTheme("");
			default:                             return {};
		}
	}

	auto TorrentCategoryModel::Category(const torrent & torr) noexcept -> category_set
	{
		category_set result;
		const auto status = torr.status();
		const auto requested_size = torr.requested_size();
		const auto current_size = torr.current_size();
		const auto error_string = torr.error_string();

		const bool all = true;
		const bool error = error_string.has_value();
		const bool stopped = status == torrent_status::stopped;
		const bool active = not stopped and not error;
		const bool nonactive = not stopped;
		const bool downloading = status == torrent_status::downloading or status == torrent_status::downloading_queued;
		const bool downloaded = current_size == requested_size;

		result.set(category_type::all, all);
		result.set(category_type::error, error);
		result.set(category_type::stopped, stopped);
		result.set(category_type::active, active);
		result.set(category_type::nonactive, nonactive);
		result.set(category_type::downloading, downloading);
		result.set(category_type::downloaded, downloaded);

		return result;
	}

	void TorrentCategoryModel::InitCategoryItems()
	{
		m_categories.push_back({.category = all});
		m_categories.push_back({.category = downloading});
		m_categories.push_back({.category = downloaded});
		m_categories.push_back({.category = active});
		m_categories.push_back({.category = nonactive});
		m_categories.push_back({.category = stopped});
		m_categories.push_back({.category = error});

		for (auto & cat : m_categories)
		{
			cat.count = 0;
			cat.icon = IconForCategory(cat.category);
			cat.name = NameForCategory(cat.category);
		}
	}

	void TorrentCategoryModel::Reinit()
	{
		beginResetModel();
		InitCategoryItems();
		RecalculateData();
		endResetModel();
	}

	void TorrentCategoryModel::RecalculateData()
	{
		for (auto & category_item : m_categories)
			category_item.count =  0;

		if (m_torrent_store)
		{
			for (const torrent & tor : *m_torrent_store)
			{
				auto catset = Category(tor);

				if (catset[all])         m_categories[0].count += 1;
				if (catset[downloading]) m_categories[1].count += 1;
				if (catset[downloaded])  m_categories[2].count += 1;
				if (catset[active])      m_categories[3].count += 1;
				if (catset[nonactive])   m_categories[4].count += 1;
				if (catset[stopped])     m_categories[5].count += 1;
				if (catset[error])       m_categories[6].count += 1;
			}
		}

		int first = 0;
		int last  = qint(m_categories.size() - 1);
		Q_EMIT dataChanged(index(first), index(last));
	}

	void TorrentCategoryModel::SetTorrentStore(std::shared_ptr<torrent_store> store)
	{
		if (store)
		{
			m_torrent_store = std::move(store);

			auto callback = [this](auto && ...) { RecalculateData(); };
			m_on_update_conn = m_torrent_store->on_update(callback);
			m_on_erase_conn = m_torrent_store->on_erase(callback);
			m_on_clear_conn = m_torrent_store->on_clear(callback);
		}
		else
		{
			m_on_update_conn.disconnect();
			m_on_erase_conn.disconnect();
			m_on_clear_conn.disconnect();

			m_torrent_store = std::move(store);
		}

		RecalculateData();
	}

	int TorrentCategoryModel::rowCount(const QModelIndex & parent) const
	{
		return qint(m_categories.size());
	}

	auto TorrentCategoryModel::GetItem(int row) const -> category_item
	{
		return m_categories.at(row);
	}

	QVariant TorrentCategoryModel::data(const QModelIndex & index, int role) const
	{
		int row = index.row();
		if (row >= qint(m_categories.size())) return {};

		auto loc = QLocale();
		const auto & item = GetItem(row);
		switch (role)
		{
			case Qt::DisplayRole:    return QStringLiteral("%1 (%2)").arg(item.name).arg(loc.toString(static_cast<qulonglong>(item.count)));
			case Qt::DecorationRole: return item.icon;
			default: return QVariant();
		}
	}

	TorrentCategoryModel::TorrentCategoryModel(QObject * parent)
	    : QAbstractListModel(parent)
	{
		Reinit();
	}
}
