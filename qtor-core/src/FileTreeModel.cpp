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
		    case torrent_file_meta::Index:
		    default:

			case torrent_file_meta::FilePath:
		    case torrent_file_meta::FileName:
			    m_leaf_compare = COMPARE(leaf_type, path_type, filename);
				m_node_compare = COMPARE(node_type, path_type, filename);
			    break;

		    case torrent_file_meta::TotalSize:
			    m_leaf_compare = COMPARE(leaf_type, size_type, total_size);
				m_node_compare = COMPARE(node_type, size_type, total_size);
			    break;

		    case torrent_file_meta::HaveSize:
			    m_leaf_compare = COMPARE(leaf_type, size_type, have_size);
				m_node_compare = COMPARE(node_type, size_type, have_size);
			    break;

		    case torrent_file_meta::Priority:
			    m_leaf_compare = COMPARE(leaf_type, int_type, priority);
				m_node_compare = COMPARE(node_type, int_type, priority);
			    break;

		    case torrent_file_meta::Wanted:
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

	bool torrent_file_tree_traits::filepath_filter::matches(const QString & rec) const noexcept
	{
		return rec.contains(m_filterStr, Qt::CaseInsensitive);
	}


	QVariant FileTreeModelBase::GetItem(const QModelIndex & idx) const
	{
		if (not idx.isValid()) return QVariant();

		const auto & val = get_ielement_ptr(idx);
		const auto meta_index = ViewToMetaIndex(idx.column());

		auto visitor = [this, meta_index](auto * ptr)
		{
			auto * meta = dynamic_cast<const torrent_file_meta *>(this->m_meta.get());
			return meta->get_item(*ptr, meta_index);
		};

		return viewed::visit(node_accessor(visitor), val);
	}

	struct any_from_element
	{
		auto operator()(const torrent_file * ptr) const { return QVariant::fromValue(ptr); }
		auto operator()(const torrent_dir * ptr)  const { return QVariant::fromValue(ptr); }
	};

	QVariant FileTreeModelBase::GetEntity(const QModelIndex & idx) const
	{
		if (not idx.isValid()) return QVariant();

		const auto & val = get_ielement_ptr(idx);
		return viewed::visit(node_accessor(any_from_element()), val);
	}

	int FileTreeModelBase::FullRowCount(const QModelIndex & idx) const
	{
		if (not idx.isValid()) return 0;

		const auto * page = get_page(idx);
		return page->children.size();
	}

	void FileTreeModelBase::recalculate_page(page_type & page)
	{
		constexpr size_type zero = 0;
		auto & children = page.children.template get<view_type::by_seq>();
		auto first = children.begin();
		auto last  = children.end();
		
		page.node.total_size = std::accumulate(first, last, zero, [](size_type val, auto & item) { return val + get_total_size(item); });
		page.node.have_size  = std::accumulate(first, last, zero, [](size_type val, auto & item) { return val + get_have_size(item);  });
	}

	void FileTreeModelBase::SortBy(int column, Qt::SortOrder order)
	{
		sort_by(ViewToMetaIndex(column), order);
	}

	void FileTreeModelBase::FilterBy(QString expr)
	{
		filter_by(expr);
	}

	FileTreeModelBase::FileTreeModelBase(QObject * parent)
	    : base_type(parent)
	{
		m_meta = std::make_shared<torrent_file_meta>();
		m_fmt = std::make_shared<formatter>();

		SetColumns({
		    torrent_file_meta::FileName,
		    torrent_file_meta::TotalSize,
		    torrent_file_meta::HaveSize
		});
	}
}
