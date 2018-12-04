#pragma once
#include <qtor/model_meta.hpp>

namespace qtor
{
	template <class Type>
	class custom_meta : public model_accessor<Type>
	{
		using base_type = model_accessor<Type>;

	public:
		using typename base_type::index_type;
		using typename base_type::any_type;

	protected:
		struct item_meta
		{
			unsigned bound : 1;
			unsigned type : sizeof(unsigned) * CHAR_BIT - 1;
			index_type wrapped_index;
			string_type name;
			any_type bound_value;
		};

	protected:
		const model_accessor<Type> * m_wrapped = nullptr;
		std::vector<item_meta> m_meta;

	protected: // model_meta interface
		item_meta get_wrapped_meta(index_type index) const;

	public:
		virtual index_type item_count() const noexcept override { return m_meta.size(); }
		virtual unsigned item_type(index_type index) const noexcept override { return m_meta[index].type; }
		virtual string_type item_name(index_type index) const override { return m_meta[index].name; }
		virtual bool is_virtual_item(index_type index) const override { return m_meta[index].bound; }

		virtual any_type get_item(const Type & item, index_type index) const override;
		virtual void     set_item(Type & item, index_type index, const any_type & val) const override;

	public: // customization interface
		virtual void bound_value(index_type where, string_type name, unsigned type, any_type value);
		virtual void unbound_value(index_type where);
		virtual void remove_item(index_type where);
		virtual void reset_item(index_type original_index);

		virtual void insert_bounded_value(index_type before, string_type name, unsigned type, any_type value);
		virtual void push_bound_value(string_type name, unsigned type, any_type value);

	public:
		virtual void reset();
		virtual void reset(const model_accessor<Type> & wrapped);

	public:
		custom_meta() = default;
		custom_meta(const model_accessor<Type> & wrapped);
	};


	template <class Type>
	auto custom_meta<Type>::get_item(const Type & item, index_type index) const -> any_type
	{
		auto & meta = m_meta[index];
		if (meta.bound) return meta.bound_value;

		return m_wrapped->get_item(item, meta.wrapped_index);
	}

	template <class Type>
	void custom_meta<Type>::set_item(Type & item, index_type index, const any_type & val) const
	{
		auto & meta = m_meta[index];
		if (meta.bound) return;

		return m_wrapped->set_item(item, meta.wrapped_index, val);
	}

	template <class Type>
	auto custom_meta<Type>::get_wrapped_meta(index_type index) const -> item_meta
	{
		item_meta item;
		item.type = m_wrapped->item_type(index);
		item.name = m_wrapped->item_name(index);
		item.wrapped_index = index;
		item.bound = 0;

		return item;
	}

	template <class Type>
	void custom_meta<Type>::bound_value(index_type where, string_type name, unsigned type, any_type value)
	{
		assert(where < m_meta.size());
		auto & item = m_meta[where];

		item.bound = 1;
		item.type = type;
		item.name = std::move(name);
		item.bound_value = std::move(value);
	}

	template <class Type>
	void custom_meta<Type>::unbound_value(index_type where)
	{
		assert(where < m_meta.size());
		auto & item = m_meta[where];

		if (item.wrapped_index == -1)
		{
			m_meta.erase(m_meta.begin() + where);
		}
		else
		{
			item.bound = 0;
			item.type = m_wrapped->item_type(item.wrapped_index);
			item.name = m_wrapped->item_name(item.wrapped_index);
			item.bound_value.clear();
		}
	}

	template <class Type>
	void custom_meta<Type>::remove_item(index_type where)
	{
		m_meta.erase(m_meta.begin() + where);
	}

	template <class Type>
	void custom_meta<Type>::reset_item(index_type original_index)
	{
		auto first = m_meta.begin();
		auto last  = m_meta.end();
		auto it = std::lower_bound(first, last, original_index, [](auto & val, auto index) { return val.wrapped_index < index; });

		if (it == last or it->wrapped_index != original_index)
		{
			m_meta.insert(it, get_wrapped_meta(original_index));
		}
		else
		{
			*it = get_wrapped_meta(original_index);
		}
	}

	template <class Type>
	void custom_meta<Type>::insert_bounded_value(index_type before, string_type name, unsigned type, any_type value)
	{
		assert(before <= m_meta.size());

		item_meta item;
		item.bound = 1;
		item.type = type;
		item.name = std::move(name);
		item.bound_value = std::move(value);
		item.wrapped_index = -1;

		m_meta.insert(m_meta.begin() + before, std::move(item));
	}

	template <class Type>
	void custom_meta<Type>::push_bound_value(string_type name, unsigned type, any_type value)
	{
		return insert_bounded_value(m_meta.size(), std::move(name), type, std::move(value));
	}

	template <class Type>
	void custom_meta<Type>::reset()
	{
		m_wrapped = nullptr;
		m_meta.clear();
	}

	template <class Type>
	void custom_meta<Type>::reset(const model_accessor<Type> & wrapped)
	{
		m_wrapped = &wrapped;

		auto count = m_wrapped->item_count();
		m_meta.resize(count);

		for (index_type u = 0; u < count; ++u)
			m_meta[u] = get_wrapped_meta(u);
	}

	template <class Type>
	custom_meta<Type>::custom_meta(const model_accessor<Type> & wrapped)
	{
		reset(wrapped);
	}
}
