#include <qtor/qt_model_meta.hpp>
#include <ext/functors/ctpred.hpp>

namespace qtor
{
	auto qt_model_meta<void>::item_count() const noexcept -> index_type
	{
		return m_item_meta.size();
	}

	unsigned qt_model_meta<void>::item_type(index_type index) const noexcept
	{
		return m_item_meta[index].type;
	}

	auto qt_model_meta<void>::item_name(index_type index) const -> string_type
	{
		return m_item_meta[index].name;
	}

	bool qt_model_meta<void>::is_virtual_item(index_type index) const
	{
		return m_item_meta[index].is_virtual;
	}

	auto qt_model_meta<void>::get_item(const void * item, index_type index) const -> any_type
	{
		const QMetaProperty & prop_meta = m_item_meta[index].property;
		return prop_meta.readOnGadget(item);
	}

	void qt_model_meta<void>::set_item(void * item, index_type index, const any_type & val) const
	{
		const QMetaProperty & prop_meta = m_item_meta[index].property;
		bool result = prop_meta.writeOnGadget(item, val);
		Q_ASSERT(result);
	}

	QStringRef extract_type(QStringRef type_name)
	{
		return type_name.mid(type_name.lastIndexOf(':'));
	}

	static unsigned item_type(QStringRef type_name)
	{
		type_name = extract_type(type_name);

#define MAP(name, type) if (type_name.contains(name, Qt::CaseInsensitive)) return model_meta::type

		MAP("speed_type", Speed);
		MAP("size_type",  Size);
		MAP("int64_type", Int64);
		MAP("uint64_type", Uint64);

		MAP("bool", Bool);
		MAP("double", Double);
		MAP("ratio_type", Ratio);
		MAP("percent_type", Percent);

		MAP("string_type", String);
		MAP("QString", String);

		MAP("datetime_type", DateTime);
		MAP("time_point", DateTime);
		MAP("QDateTime", DateTime);
		MAP("QDate", DateTime);

		MAP("duration_type", Duration);

#undef MAP

		return model_meta::Unknown;
	}

	qt_model_meta<void>::qt_model_meta(const QMetaObject & meta)
	{
		auto n = meta.propertyCount();
		m_item_meta.resize(n);

		for (int i = 0; i < n; ++i)
		{
			auto & item = m_item_meta[i];
			QMetaProperty & property = item.property = meta.property(i);
			item.name = property.name();
			item.type_name = property.typeName();
			item.type = qtor::item_type(&item.type_name);
			item.is_virtual = not property.isStored();
		}
	}
}
