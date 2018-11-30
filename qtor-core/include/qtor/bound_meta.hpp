#pragma once
#include <qtor/model_meta.hpp>

namespace qtor
{
	template <class Type>
	class torrent_meta_adapter : public model_accessor<Type>
	{
		using base_type = model_accessor<Type>;

	public:
		using typename base_type::index_type;
		using typename base_type::any_type;

	protected:
		struct item_meta
		{
			unsigned type;
			string_type name;
			index_type wrapped_index;
			any_type bound_value;
		};

	protected:
		const model_accessor<Type> * m_wrapped;
		std::vector<item_meta> m_meta;

		// model_meta interface
	public:
		virtual index_type item_count() const noexcept override { return m_meta.size(); }
		virtual unsigned item_type(index_type index) const noexcept override;
		virtual string_type item_name(index_type index) const override;
		virtual bool is_virtual_item(index_type index) const override;

		virtual any_type get_item(const Type & item, index_type index) const override;
		virtual void     set_item(Type & item, index_type index, const any_type & val) const override;

	public:
		virtual void insert_bounded_value(index_type before, string_type name, unsigned type, any_type value);
		virtual void bound_value(index_type where, string_type name, unsigned type, any_type value);
		virtual void unbound_value(index_type where);
		virtual void push_bound_value(string_type name, unsigned type, any_type value);

	public:
		torrent_meta_adapter(const model_accessor<Type> & wrapped);
	};
}
