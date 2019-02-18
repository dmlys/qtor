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

	auto torrent_file_tree_traits::parse_path(const pathview_type & path, const pathview_type & context) const
	    -> std::tuple<std::uintptr_t, pathview_type, pathview_type>
	{
		//[first, last) - next segment in leaf_path,
		auto first = path.begin() + context.size();
		auto last = path.end();
		auto it = std::find(first, last, '/');

		if (it == last)
		{
			pathview_type name = path.mid(context.size());
			return std::make_tuple(viewed::LEAF, context, std::move(name));
		}
		else
		{
			pathview_type name = path.mid(context.size(), it - first);
			it = std::find_if_not(it, last, [](auto ch) { return ch == '/'; });
			pathview_type newpath = path.left(it - path.begin());
			return std::make_tuple(viewed::PAGE, std::move(newpath), std::move(name));
		}
	}

	bool torrent_file_tree_traits::is_child(const pathview_type & path, const pathview_type & context, const pathview_type & node_name) const
	{
		auto ref = path.mid(context.size(), node_name.size());
		return ref == node_name;
	}

	QVariant FileTreeModelBase::GetItem(const QModelIndex & idx) const
	{
		if (not idx.isValid()) return QVariant();

		const auto & val = get_element_ptr(idx);
		const auto meta_index = ViewToMetaIndex(idx.column());

		auto visitor = [this, meta_index](auto * ptr)
		{
			auto * meta = dynamic_cast<const torrent_file_meta *>(this->m_meta.get());
			return meta->get_item(*ptr, meta_index);
		};

		return viewed::visit(visitor, val);
	}

	struct any_from_element
	{
		auto operator()(const torrent_file * ptr) const { return QVariant::fromValue(ptr); }
		auto operator()(const torrent_dir * ptr)  const { return QVariant::fromValue(ptr); }
	};

	QVariant FileTreeModelBase::GetEntity(const QModelIndex & idx) const
	{
		if (not idx.isValid()) return QVariant();

		const auto & val = get_element_ptr(idx);
		return viewed::visit(any_from_element(), val);
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
		
		page.total_size = std::accumulate(first, last, zero, [](size_type val, auto & item) { return val + get_total_size(item); });
		page.have_size  = std::accumulate(first, last, zero, [](size_type val, auto & item) { return val + get_have_size(item);  });
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
	}

	FileTreeModel::FileTreeModel(QObject * parent /* = nullptr */)
	    : base_type(parent)
	{
		SetColumns({
		    torrent_file_meta::FileName,
		    torrent_file_meta::TotalSize,
		    torrent_file_meta::HaveSize
		});
	}

	FileTreeViewModel::FileTreeViewModel(std::shared_ptr<torrent_file_store> store, QObject * parent /* = nullptr */)
	    : base_type(std::move(store), parent)
	{
		SetColumns({
		    torrent_file_meta::FileName,
		    torrent_file_meta::TotalSize,
		    torrent_file_meta::HaveSize
		});
	}
}
