#pragma once
#include <qtor/types.hpp>
#include <qtor/model_meta.hpp>

#include <QtCore/QMetaProperty>
#include <QtCore/QMetaObject>

namespace qtor
{
	template <class Type>
	class qt_model_meta;

	template <>
	class qt_model_meta<void> : virtual model_meta
	{
		using self_type = qt_model_meta;
		using base_type = model_meta;

	protected:
		struct item_description
		{
			string_type name;
			string_type type_name;
			QMetaProperty property;
			unsigned type;
			bool is_virtual;
		};

	protected:
		std::vector<item_description> m_item_meta;

	public:
		virtual  index_type item_count()                const noexcept override;
		virtual    unsigned item_type(index_type index) const noexcept override;
		virtual string_type item_name(index_type index) const          override;
		virtual bool is_virtual_item(index_type index)  const          override;

	protected:
		any_type get_item(const void * item, index_type index) const;
		void     set_item(void * item, index_type index, const any_type & val) const;

	protected:
		qt_model_meta(const QMetaObject & meta);
	};


	template <class Type>
	class qt_model_meta : public qt_model_meta<void>,
	                      public model_accessor<Type>
	{
		using self_type = qt_model_meta;
		using base_type = qt_model_meta<void>;

	public:
		virtual any_type get_item(const Type & item, index_type index)                 const override;
		virtual void     set_item(Type & item, index_type index, const any_type & val) const override;

	public:
		qt_model_meta() : base_type(Type::staticMetaObject) {}
	};


	template <class Type>
	auto qt_model_meta<Type>::get_item(const Type & item, index_type index) const -> any_type
	{
		return base_type::get_item(&item, index);
	}

	template <class Type>
	void qt_model_meta<Type>::set_item(Type & item, index_type index, const any_type & val) const
	{
		return base_type::set_item(&item, index, val);
	}
}
