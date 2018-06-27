#include <qtor/FileTreeModel.hqt>

namespace qtor
{
	template <class Entity, class Type, Type Entity::*member, class Pred>
	bool torrent_file_tree_traits::column_sorter::compare_entity(const Entity & e1, const Entity & e2) noexcept
	{
		Pred pred;
		const auto & v1 = e1.*member;
		const auto & v2 = e2.*member;
		return pred(v1, v2);
	}

	void torrent_file_tree_traits::column_sorter::reset(unsigned type, Qt::SortOrder order)
	{
#define COMPARE(ENTITY, TYPE, MEMBER)                                           \
		order == Qt::AscendingOrder                                             \
			? compare_entity<ENTITY, TYPE, &ENTITY::MEMBER, std::less<>>        \
			: compare_entity<ENTITY, TYPE, &ENTITY::MEMBER, std::greater<>>		\

		switch (type)
		{
			case torrent_file::Index:
			default:

			case torrent_file::FileName:
				m_leaf_compare = COMPARE(leaf_type, path_type, filename);
				m_node_compare = COMPARE(node_type, path_type, name);
				break;

			case torrent_file::TotalSize:
				m_leaf_compare = COMPARE(leaf_type, size_type, total_size);
				m_node_compare = COMPARE(node_type, size_type, total_size);
				break;

			case torrent_file::HaveSize:
				m_leaf_compare = COMPARE(leaf_type, size_type, have_size);
				m_node_compare = COMPARE(node_type, size_type, have_size);
				break;

			case torrent_file::Priority:
				m_leaf_compare = COMPARE(leaf_type, int_type, priority);
				m_node_compare = COMPARE(node_type, int_type, priority);
				break;

			case torrent_file::Wanted:
				m_leaf_compare = COMPARE(leaf_type, bool, wanted);
				m_node_compare = COMPARE(node_type, Qt::CheckState, wanted);
				break;
		}

#undef COMPARE
	}

	viewed::refilter_type torrent_file_tree_traits::filepath_filter::set_expr(QString expr)
	{
		expr = expr.trimmed();
		if (expr.compare(m_filterStr, Qt::CaseInsensitive) == 0)
			return viewed::refilter_type::same;

		if (expr.startsWith(m_filterStr, Qt::CaseInsensitive))
		{
			m_filterStr = expr;
			return viewed::refilter_type::incremental;
		}
		else
		{
			m_filterStr = expr;
			return viewed::refilter_type::full;
		}
	}

	bool torrent_file_tree_traits::filepath_filter::matches(const QString & rec) const
	{
		return rec.contains(m_filterStr, Qt::CaseInsensitive);
	}

	filepath_type torrent_file_tree_traits::get_segment(const filepath_type & filepath)
	{
		int pos = filepath.lastIndexOf('/') + 1;
		return filepath.mid(pos);
	}

	auto torrent_file_tree_traits::analyze(const pathview_type & prefix, const leaf_type & item)
		-> std::tuple<std::uintptr_t, path_type, pathview_type>
	{
		const auto & path = item.filename;
		auto first = path.begin() + prefix.size();
		auto last = path.end();
		auto it = std::find(first, last, '/');

		if (it == last)
		{
			QString name = QString::null;
			return std::make_tuple(viewed::LEAF, std::move(name), prefix);
		}
		else
		{
			QString name = path.mid(prefix.size(), it - first);
			it = std::find_if_not(it, last, [](auto ch) { return ch == '/'; });
			return std::make_tuple(viewed::PAGE, std::move(name), path.leftRef(it - path.begin()));
		}
	}

	bool torrent_file_tree_traits::is_subelement(const pathview_type & prefix, const path_type & name, const leaf_type & item)
	{
		auto ref = item.filename.midRef(prefix.size(), name.size());
		return ref == name;
	}



	QVariant FileTreeModel::GetItem(const QModelIndex & idx) const
	{
		return GetValueShort(idx);
	}

	QString FileTreeModel::GetValue(const QModelIndex & idx) const
	{
		auto & val = get_element_ptr(idx);
		auto type = m_columns[idx.column()];

		switch (type)
		{
			case torrent_file::FileName:  return m_fmt->format_string(get_segment(val));
			case torrent_file::TotalSize: return m_fmt->format_size(get_total_size(val));
			case torrent_file::HaveSize:  return m_fmt->format_size(get_have_size(val));
			case torrent_file::Index:     return QStringLiteral("<null>");
			case torrent_file::Priority:  return QStringLiteral("<null>");
			case torrent_file::Wanted:    return QStringLiteral("<null>");
			default:                      return QStringLiteral("<null>");
		}
	}

	QString FileTreeModel::GetValueShort(const QModelIndex & idx) const
	{
		auto & val = get_element_ptr(idx);
		auto type = m_columns[idx.column()];

		switch (type)
		{
			case torrent_file::FileName:  return m_fmt->format_short_string(get_segment(val));
			case torrent_file::TotalSize: return m_fmt->format_size(get_total_size(val));
			case torrent_file::HaveSize:  return m_fmt->format_size(get_have_size(val));
			case torrent_file::Index:     return QStringLiteral("<null>");
			case torrent_file::Priority:  return QStringLiteral("<null>");
			case torrent_file::Wanted:    return QStringLiteral("<null>");
			default:                      return QStringLiteral("<null>");
		}
	}

	void FileTreeModel::recalculate_page(page_type & page)
	{
		constexpr size_type zero = 0;
		auto & children = page.children.get<by_seq>();
		auto first = children.begin();
		auto last  = children.end();
		
		page.total_size = std::accumulate(first, last, zero, [](size_type val, auto & item) { return val + get_total_size(item); });
		page.have_size  = std::accumulate(first, last, zero, [](size_type val, auto & item) { return val + get_have_size(item); });
	}

	void FileTreeModel::SortBy(int column, Qt::SortOrder order)
	{
		sort_by(column, order);
	}

	void FileTreeModel::FilterBy(QString expr)
	{
		filter_by(expr);
	}

	//FileTreeModel::FileTreeModel(std::shared_ptr<torrent_file_store> store, QObject * parent /* = nullptr */)
	//	: base_type(std::move(store), parent)
	//{
	//	init();
	//	InitColumns();
	//	m_fmt = new formatter(this);
	//}

	FileTreeModel::FileTreeModel(QObject * parent /* = nullptr */)
		: base_type(parent)
	{
		InitColumns();
		m_fmt = new formatter(this);
	}
}
