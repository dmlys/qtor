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
			unsigned bound : 1;
			unsigned type : sizeof(unsigned) * CHAR_BIT - 1;
			index_type wrapped_index;
			string_type name;
			any_type bound_value;
		};

	protected:
		const model_accessor<Type> * m_wrapped;
		std::vector<item_meta> m_meta;

		// model_meta interface
	public:
		virtual index_type item_count() const noexcept override { return m_meta.size(); }
		virtual unsigned item_type(index_type index) const noexcept override { return m_meta[index].type; }
		virtual string_type item_name(index_type index) const override { return m_meta[index].name; }
		virtual bool is_virtual_item(index_type index) const override { return m_meta[index].bound; }

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


	template <class Type>
	auto torrent_meta_adapter<Type>::get_item(const Type & item, index_type index) const -> any_type
	{
		auto & meta = m_meta[index];
		if (meta.bound) return meta.bound_value;

		return m_wrapped->get_item(item, meta.wrapped_index);
	}

	template <class Type>
	void torrent_meta_adapter<Type>::set_item(Type & item, index_type index, const any_type & val) const
	{
		auto & meta = m_meta[index];
		if (meta.bound) return;

		return m_wrapped->set_item(item, meta.wrapped_index, val);
	}

	template <class Type>
	void torrent_meta_adapter<Type>::bound_value(index_type where, string_type name, unsigned type, any_type value)
	{
		assert(where < m_meta.size());
		auto & item = m_meta[where];

		item.bound = 1;
		item.type = type;
		item.name = std::move(name);
		item.bound_value = std::move(value);
	}

	template <class Type>
	void torrent_meta_adapter<Type>::unbound_value(index_type where)
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
	void torrent_meta_adapter<Type>::insert_bounded_value(index_type before, string_type name, unsigned type, any_type value)
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
	void torrent_meta_adapter<Type>::push_bound_value(string_type name, unsigned type, any_type value)
	{
		return insert_bounded_value(m_meta.size(), std::move(name), type, std::move(value));
	}

	template <class Type>
	torrent_meta_adapter<Type>::torrent_meta_adapter(const model_accessor<Type> & wrapped)
	    : m_wrapped(&wrapped)
	{
		auto count = m_wrapped->item_count();
		m_meta.resize(count);
		for (index_type u = 0; u < count; ++u)
		{
			auto & item = m_meta[u];
			item.type = m_wrapped->item_type(u);
			item.name = m_wrapped->item_name(u);
			item.wrapped_index = u;
			item.bound = 0;
		}
	}
}
