#pragma once
#include <bitset>
#include <tuple>
#include <unordered_map>
#include <qtor/types.hpp>
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
		using any_type   = QVariant;

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
	class sparse_container_meta
	{
	public:
		using index_type = unsigned;
		using any_type = any;

	public:
		// currently supported types,
		// it's not enum to allow user custom extension
		static constexpr unsigned Uint64   = 0;
		static constexpr unsigned Int64    = 1;
		static constexpr unsigned Bool     = 2;
		static constexpr unsigned Double   = 3;
		static constexpr unsigned String   = 4;

		static constexpr unsigned Speed    = 5;
		static constexpr unsigned Size     = 6;
		static constexpr unsigned DateTime = 7;
		static constexpr unsigned Duration = 8;
		static constexpr unsigned Percent  = 9;
		static constexpr unsigned Ratio    = 10;

		static constexpr unsigned Unknown = -1;

	public:
		virtual index_type items_count() const noexcept = 0;
		virtual unsigned item_type(index_type key) const noexcept = 0;

	public:
		virtual ~sparse_container_meta() = default;
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
