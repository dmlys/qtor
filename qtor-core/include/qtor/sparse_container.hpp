#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <qtor/types.hpp>
#include <qtor/model_meta.hpp>
#include <viewed/refilter_type.hpp>

namespace qtor
{
	class formatter;

	/************************************************************************/
	/*                     sparse_container                                 */
	/************************************************************************/
	class sparse_container
	{
	public:
		using index_type = unsigned;
		using any_type   = any;

	protected:
		std::unordered_map<index_type, any_type> m_items;

	public:
		static const any_type ms_empty;

	public:
		const auto & items() const { return m_items; }

		void remove_item(index_type key);
		
		auto set_item(index_type key, any_type val)  -> sparse_container &;
		template <class Type> auto set_item(index_type key, Type val)           -> sparse_container &;
		template <class Type> auto set_item(index_type key, optional<Type> val) -> sparse_container &;
		
		auto get_item(index_type key)       ->       any_type &;
		auto get_item(index_type key) const -> const any_type &;

		//template <class Type> optional<Type &>       get_item(index_type key);
		template <class Type> optional<Type> get_item(index_type key) const;
	};


	/************************************************************************/
	/*                    sparse_container_meta                             */
	/************************************************************************/
	class simple_sparse_container_meta : public model_accessor<sparse_container>
	{
		using base_type = model_meta;
		using self_type = simple_sparse_container_meta;

	public:
		struct item
		{
			unsigned      type;
			string_type   name;
		};

		using item_map     = std::unordered_map<index_type, item>;
		using item_map_ptr = std::shared_ptr<const item_map>;

	protected:
		item_map_ptr m_items;

	public:
		virtual  index_type item_count()               const noexcept override;
		virtual    unsigned item_type(index_type key)  const noexcept override;
		virtual string_type item_name(index_type key)  const          override;
		virtual bool is_virtual_item(index_type index) const          override;

		virtual any_type get_item(const sparse_container & item, index_type key) const override;
		virtual void     set_item(sparse_container & item, index_type key, const any_type & val) const override;

	public:
		simple_sparse_container_meta(item_map_ptr items)
			: m_items(std::move(items)) {}

		virtual ~simple_sparse_container_meta() = default;
	};


	/************************************************************************/
	/*                    sparse_container_comparator                       */
	/************************************************************************/
	class sparse_container_comparator
	{
		sparse_container::index_type m_key = 0;
		bool m_ascending = true;

	public:
		bool operator()(const sparse_container & c1, const sparse_container & c2) const;

		sparse_container_comparator() = default;
		sparse_container_comparator(sparse_container::index_type key, bool ascending)
			: m_key(key), m_ascending(ascending) {}
	};


	/************************************************************************/
	/*                     sparse_container_filter                          */
	/************************************************************************/
	class sparse_container_filter
	{
	public:
		using index_array = std::vector<sparse_container::index_type>;

	private:
		index_array m_items;
		string_type m_filter;

	public:
		// same, incremental
		viewed::refilter_type set_items(index_array items);
		viewed::refilter_type set_expr(string_type search);
		viewed::refilter_type set_expr(string_type search, index_array items);

		bool matches(const sparse_container & c) const;
		bool matches(const sparse_container::any_type & val) const;
		bool always_matches() const noexcept;

		bool operator()(const sparse_container & c) const { return matches(c); }
		explicit operator bool() const noexcept { return not always_matches(); }
	};



	/************************************************************************/
	/*                   inline and template methods                        */
	/************************************************************************/
	inline void sparse_container::remove_item(index_type key)
	{
		m_items.erase(key);
	}

	template <class Type>
	inline auto sparse_container::set_item(index_type key, Type val) -> sparse_container &
	{
		return set_item(key, any_type::fromValue(std::move(val)));
	}

	template <class Type>
	inline auto sparse_container::set_item(index_type key, optional<Type> val) -> sparse_container &
	{
		if (val)
			return set_item(key, any_type::fromValue(std::move(val).value()));
		else
		{
			remove_item(key);
			return *this;
		}
	}

	inline auto sparse_container::set_item(index_type key, any_type val) -> sparse_container &
	{
		m_items.insert_or_assign(key, val);
		return *this;
	}

	inline auto sparse_container::get_item(index_type key) -> any_type &
	{
		return m_items[key];
	}

	inline auto sparse_container::get_item(index_type key) const -> const any_type &
	{
		auto it = m_items.find(key);
		if (it == m_items.end()) return ms_empty;
		else                     return it->second;
	}

	//template <class Type>
	//optional<Type &> sparse_container::get_item(index_type key)
	//{
	//	auto it = m_items.find(key);
	//	if (it == m_items.end()) return nullopt;

	//	auto & item = it->second;
	//	auto * val = any_cast<Type>(&item);

	//	if (val == nullptr) return nullopt;
	//	else                return *val;
	//}

	template <class Type>
	optional<Type> sparse_container::get_item(index_type key) const
	{
		auto it = m_items.find(key);
		if (it == m_items.end()) return nullopt;

		auto & item = it->second;
		auto * val = any_cast<Type>(&item);

		if (val == nullptr) return nullopt;
		else                return *val;
	}
}
