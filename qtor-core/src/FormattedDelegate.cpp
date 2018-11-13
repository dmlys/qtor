#include <qtor/FormattedDelegate.hqt>
#include <ext/utility.hpp>

namespace qtor
{
	void FormattedDelegate::InitStyle(QStyleOptionViewItem & option, const QModelIndex & index) const
	{
		m_model = dynamic_cast<const AbstractItemModel *>(index.model());
		//QVariant fmtvar = m_model->property("formatter");
		//QVariant metavar = m_model->property("meta");
		//m_fmt = qvariant_cast<formatter *>(fmtvar);
		//m_meta = qvariant_cast<model_meta *>(metavar);

		m_fmt = m_model->GetFormatter().get();
		m_meta = m_model->GetMeta().get();
		m_cur_idx = index;

		base_type::InitStyle(option, index);
	}

	QString FormattedDelegate::displayText(const QVariant & value, const QLocale & locale) const
	{
		auto old_locale = m_fmt->get_locale();
		m_fmt->set_locale(locale);

		auto view_index = m_cur_idx.column();
		auto meta_index = m_model->ViewToMetaIndex(view_index);
		auto item_type = m_meta->item_type(meta_index);

		auto str = m_fmt->format_item_short(value, item_type);

		m_fmt->set_locale(old_locale);

		return str;
	}
}
