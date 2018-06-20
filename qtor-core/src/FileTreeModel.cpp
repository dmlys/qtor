#include <qtor/FileTreeModel.hqt>

namespace qtor
{
	viewed::refilter_type SimpleTextFilter::set_expr(QString expr)
	{
		expr = expr.trimmed();
		if (expr.compare(m_filterWord, Qt::CaseInsensitive) == 0)
			return viewed::refilter_type::same;

		if (expr.startsWith(m_filterWord, Qt::CaseInsensitive))
		{
			m_filterWord = expr;
			return viewed::refilter_type::incremental;
		}
		else
		{
			m_filterWord = expr;
			return viewed::refilter_type::full;
		}
	}

	bool SimpleTextFilter::matches(const QString & rec) const
	{
		return rec.contains(m_filterWord, Qt::CaseInsensitive);
	}

	SimpleTextFilter FileTreeModel::m_tfilt;

	filepath_type FileTreeModelTraits::get_segment(const filepath_type & filepath)
	{
		int pos = filepath.lastIndexOf('/') + 1;
		return filepath.mid(pos);
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

	auto FileTreeModel::analyze(const QStringRef & prefix, const torrent_file & item)
		-> std::tuple<std::uintptr_t, QString, QStringRef>
	{
		const auto & path = item.filename;
		auto first = path.begin() + prefix.size();
		auto last = path.end();
		auto it = std::find(first, last, '/');

		if (it == last)
		{
			QString name = QString::null;
			return std::make_tuple(LEAF, std::move(name), prefix);
		}
		else
		{
			QString name = path.mid(prefix.size(), it - first);
			it = std::find_if_not(it, last, [](auto ch) { return ch == '/'; });
			return std::make_tuple(PAGE, std::move(name), path.leftRef(it - path.begin()));
		}
	}

	bool FileTreeModel::is_subelement(const QStringRef & prefix, const QString & name, const torrent_file & item)
	{
		auto ref = item.filename.midRef(prefix.size(), name.size());
		return ref == name;
	}

	void FileTreeModel::recalculate_page(page_type & page)
	{
		constexpr size_type zero = 0;
		auto & children = page.children.get<by_seq>();
		auto first = children.begin();
		auto last  = children.end();
		
		page.total_size = std::accumulate(first, last, zero, [](size_type val, auto & item) { return val + get_total_size(item); });
		page.have_size = std::accumulate(first, last, zero, [](size_type val, auto & item) { return val + get_have_size(item); });
	}

	void FileTreeModel::SortBy(int column, Qt::SortOrder order)
	{
		//sort_and_notify();
	}

	void FileTreeModel::FilterBy(QString expr)
	{
		auto rtype = m_tfilt.set_expr(expr);
		refilter_and_notify(rtype);
	}

	FileTreeModel::FileTreeModel(std::shared_ptr<torrent_file_store> store, QObject * parent /* = nullptr */)
		: base_type(std::move(store), parent)
	{
		init();
		InitColumns();
		m_fmt = new formatter(this);
	}
}
