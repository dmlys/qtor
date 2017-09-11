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
		const auto & items() const noexcept { return m_items; }

		void remove_item(index_type key) noexcept;
		
		template <class Type>
		auto set_item(index_type key, Type val)      -> sparse_container &;
		auto set_item(index_type key, any_type val)  -> sparse_container &;
		
		auto get_item(index_type key) noexcept       -> any_type &;
		auto get_item(index_type key) const noexcept -> const any_type &;

		template <class Type> optional<Type &>       get_item(index_type key) noexcept;
		template <class Type> optional<const Type &> get_item(index_type key) const noexcept;
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
		static constexpr unsigned Uint64 = 0;
		static constexpr unsigned Double = 1;
		static constexpr unsigned String = 2;

		static constexpr unsigned Speed = 3;
		static constexpr unsigned Size = 4;
		static constexpr unsigned DateTime = 5;
		static constexpr unsigned Duration = 6;

		static constexpr unsigned Unknown = -1;

	public:
		virtual unsigned item_type(index_type key) const noexcept = 0;
		virtual  QString item_name(index_type key) const = 0;
		virtual  QString format_item(index_type key, const any_type & val) const = 0;
		virtual  QString format_item_short(index_type key, const any_type & val) const = 0;

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
		bool operator()(const sparse_container & c1, const sparse_container & c2) const noexcept;

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

		bool matches(const sparse_container & c) const noexcept;
		bool matches(const sparse_container::any_type & val) const noexcept;
		bool always_matches() const noexcept;

		bool operator()(const sparse_container & c) const noexcept { return matches(c); }
		explicit operator bool() const noexcept { return always_matches(); }
	};



	/************************************************************************/
	/*                   inline and template methods                        */
	/************************************************************************/
	inline void sparse_container::remove_item(index_type key) noexcept
	{
		m_items.erase(key);
	}

	template <class Type>
	inline auto sparse_container::set_item(index_type key, Type val) -> sparse_container &
	{
		return set_item(key, any_type::fromValue(std::move(val)));
	}

	inline auto sparse_container::set_item(index_type key, any_type val) -> sparse_container &
	{
		m_items.insert_or_assign(key, val);
		return *this;
	}

	inline auto sparse_container::get_item(index_type key) noexcept -> any_type &
	{
		return m_items[key];
	}

	inline auto sparse_container::get_item(index_type key) const noexcept -> const any_type &
	{
		auto it = m_items.find(key);
		if (it == m_items.end()) return ms_empty;
		else                     return it->second;
	}

	template <class Type>
	optional<Type &> sparse_container::get_item(index_type key) noexcept
	{
		auto it = m_items.find(key);
		if (it == m_items.end()) return nullopt;

		auto & item = it->second;
		auto * val = any_cast<Type>(&item);

		if (val == nullptr) return nullopt;
		else                return *val;
	}

	template <class Type>
	optional<const Type &> sparse_container::get_item(index_type key) const noexcept
	{
		auto it = m_items.find(key);
		if (it == m_items.end()) return nullopt;

		auto & item = it->second;
		auto * val = any_cast<Type>(&item);

		if (val == nullptr) return nullopt;
		else                return *val;
	}
}
