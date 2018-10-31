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
//		if (m_fmt == nullptr or m_meta == nullptr)
//			return base_type::displayText(value, locale);

//		auto type = m_meta->item_type(index.column());
//		QString text; // format_item(
//		//m_fmt->format_
//		option.text = text;
	}

	QString FormattedDelegate::displayText(const QVariant & value, const QLocale & locale) const
	{
		auto old_locale = m_fmt->get_locale();
		m_fmt->set_locale(locale);
		m_fmt->set_locale(old_locale);

		return {};
	}
}
