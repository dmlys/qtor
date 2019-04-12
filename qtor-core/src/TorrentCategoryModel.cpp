#include <qtor/TorrentCategoryModel.hqt>

namespace qtor
{
	void TorrentCategoryModel::InitStatusItems()
	{
		m_categories.push_back({all, 0});
		m_categories.push_back({downloading, 0});
		m_categories.push_back({downloaded, 0});
		m_categories.push_back({active, 0});
		m_categories.push_back({nonactive, 0});
		m_categories.push_back({stopped, 0});
		m_categories.push_back({error, 0});

	}

	void TorrentCategoryModel::UpdateData(const torrent * first, const torrent * last)
	{
		for (; first != last; ++first)
		{

		}
	}
}
