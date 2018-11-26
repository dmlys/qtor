#pragma once
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/range/iterator_range.hpp>

#include <qtor/types.hpp>
#include <qtor/model_meta.hpp>

namespace qtor
{
	template <class type>
	struct field_iterator_proxy
	{
		using reference  = type &;
		using pointer    = type *;

		using index_type = model_meta::index_type;
		using any_type   = model_meta::any_type;

	private:
		const model_accessor<type> * meta;
		pointer valptr;
		index_type index;

	public:
		operator any_type() const { return meta->get_item(valptr, index); }
		field_iterator_proxy & operator =(const any_type & val) { meta->set_item(*valptr, index, val); }

	public:
		field_iterator_proxy(const model_accessor<type> * meta, pointer valptr, index_type index)
		    : meta(meta), valptr(valptr), index(index) {}
	};


	template <class index_iterator>
	struct field_iterator_category_helper
	{
		using type = typename std::iterator_traits<index_iterator>::iterator_category;
		auto index(const index_iterator & it) { return *it; }
	};

	template <>
	struct field_iterator_category_helper<model_meta::index_type>
	{
		using type = std::random_access_iterator_tag;
		auto index(model_meta::index_type idx) { return idx; }
	};


	template <class type, class index_iterator, bool is_const>
	struct field_iterator_helper
	{
		using index_type = model_meta::index_type;
		using any_type   = model_meta::any_type;
		using any_type_ref = std::conditional_t<is_const, const any_type &, any_type &>;

		using value_type = type;
		using reference  = std::conditional_t<is_const, any_type, field_iterator_proxy<type>>;
		using type_pointer = std::conditional_t<is_const, const type *, type *>;
		using iterator_category = typename field_iterator_category_helper<index_iterator>::type;

		static index_type index(const index_iterator & it) noexcept { return field_iterator_category_helper<index_iterator>::index(it); }
		static auto dereference(const model_accessor<type> * meta, const type * valptr, index_type idx) { return meta->get_item(*valptr, idx);   }
		static auto dereference(const model_accessor<type> * meta,       type * valptr, index_type idx) { return reference(meta, valptr, index); }
	};


	template <class type, class index_iterator, bool is_const>
	class field_iterator_base :
		public boost::iterator_facade<
	        field_iterator_base<type, index_iterator, is_const>,
	        typename field_iterator_helper<type, index_iterator, is_const>::value_type,
	        typename field_iterator_helper<type, index_iterator, is_const>::iterator_category,
	        typename field_iterator_helper<type, index_iterator, is_const>::reference
		>
	{
		using self_type = field_iterator_base;
		using helper = field_iterator_helper<type, index_iterator, is_const>;
		using base_type = boost::iterator_facade<
			self_type,
			typename helper::value_type,
			typename helper::iterator_category,
			typename helper::reference
		>;

		friend boost::iterator_core_access;
		friend field_iterator_base<type, index_iterator, true>;

	public:
		using type_pointer = typename helper::type_pointer;
		using index_type = model_meta::index_type;

		using typename base_type::difference_type;
		using typename base_type::reference;

	private:
		const model_accessor<type> * m_meta;
		type_pointer m_valptr;
		index_iterator m_index;

	public:
		auto index() const noexcept { return helper::index(m_index); }

	private:
		void increment() noexcept { ++m_index; }
		void decrement() noexcept { --m_index; }

		void advance(difference_type n) noexcept { m_index += n; }

		auto dereference() const -> reference { return helper::dereference(m_meta, m_valptr, m_index); }
		auto distance_to(const self_type & it) const noexcept -> difference_type { return it.m_index - m_index; }
		bool equal(const self_type & it) const noexcept { return m_valptr == it.m_valptr and m_index == it.m_index; }

	public:
		field_iterator_base(const model_accessor<type> * meta, type_pointer valptr, index_iterator idx)
		    : m_meta(meta), m_valptr(valptr), m_index(idx) {}
	};

	template <class type, class index_iterator> using const_field_iterator = field_iterator_base<type, index_iterator, true>;
	template <class type, class index_iterator> using       field_iterator = field_iterator_base<type, index_iterator, false>;

	template <class type, class index_iterator> using const_field_range = boost::iterator_range<const_field_iterator<type, index_iterator>>;
	template <class type, class index_iterator> using       field_range = boost::iterator_range<      field_iterator<type, index_iterator>>;
}

