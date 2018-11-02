#include <qtor/FormattedDelegate.hqt>
#include <ext/utility.hpp>

namespace qtor
{
	void FormattedDelegate::InitStyle(QStyleOptionViewItem & option, const QModelIndex & index) const
	{
		m_model = index.model();
		QVariant fmtvar = m_model->property("formatter");
		QVariant metavar = m_model->property("meta");
		m_fmt = qvariant_cast<formatter *>(fmtvar);
		m_meta = qvariant_cast<model_meta *>(metavar);

		base_type::InitStyle(option, index);
	}

	QString FormattedDelegate::displayText(const QVariant & value, const QLocale & locale) const
	{
		auto old_locale = m_fmt->get_locale();
		m_fmt->set_locale(locale);

		auto meta_index = 0u;
		auto view_index = 0u;

		auto str = m_fmt->format_item_short(value, meta_index);

		m_fmt->set_locale(old_locale);

		return str;
	}
}
