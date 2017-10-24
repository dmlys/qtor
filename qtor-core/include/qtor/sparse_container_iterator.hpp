#pragma once
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>

#include <qtor/types.hpp>
#include <qtor/sparse_container.hpp>


namespace qtor
{
	template <class functor, class index_iterator, bool isconst>
	struct sparse_container_transform_iterator_helper
	{
		using sparse_container_ref = std::conditional_t<isconst, const sparse_container &, sparse_container &>;
		using sparse_container_ptr = std::conditional_t<isconst, const sparse_container *, sparse_container *>;

		using index_type = sparse_container::index_type;
		using any_type = sparse_container::any_type;
		using any_type_ref = std::conditional_t<isconst, const any_type &, any_type &>;

		using reference = std::result_of_t<functor(any_type_ref, index_type)>;
		using value_type = std::decay_t<reference>;
		
		using iterator_category = typename std::iterator_traits<index_iterator>::iterator_category;
		static sparse_container::index_type index(const index_iterator & it) noexcept { return *it; }
	};

	template <class functor, bool isconst>
	struct sparse_container_transform_iterator_helper<functor, sparse_container::index_type, isconst>
	{
		using sparse_container_ref = std::conditional_t<isconst, const sparse_container &, sparse_container &>;
		using sparse_container_ptr = std::conditional_t<isconst, const sparse_container *, sparse_container *>;

		using index_type = sparse_container::index_type;
		using any_type = sparse_container::any_type;
		using any_type_ref = std::conditional_t<isconst, const any_type &, any_type &>;

		using reference = std::result_of_t<functor(any_type_ref, index_type)>;
		using value_type = std::decay_t<reference>;

		using iterator_category = std::random_access_iterator_tag;
		static sparse_container::index_type index(sparse_container::index_type idx) noexcept { return idx; }
	};


	/************************************************************************/
	/*               base_sparse_container_transform_iterator               */
	/************************************************************************/
	template <class Functor, class IndexIterator, bool isconst>
	class base_sparse_container_transform_iterator :
		public boost::iterator_facade<
			base_sparse_container_transform_iterator<Functor, IndexIterator, isconst>,
			typename sparse_container_transform_iterator_helper<Functor, IndexIterator, isconst>::value_type,
			typename sparse_container_transform_iterator_helper<Functor, IndexIterator, isconst>::iterator_category,
			typename sparse_container_transform_iterator_helper<Functor, IndexIterator, isconst>::reference
		>
	{
		using self_type = base_sparse_container_transform_iterator;
		using helper = sparse_container_transform_iterator_helper<Functor, IndexIterator, isconst>;
		using base_type = boost::iterator_facade<
			self_type,
			typename helper::value_type,
			typename helper::iterator_category,
			typename helper::reference
		>;
	
		friend boost::iterator_core_access;
		friend base_sparse_container_transform_iterator<Functor, IndexIterator, true>;
	
	public:
		using sparse_container_ref = typename helper::sparse_container_ref;
		using sparse_container_ptr = typename helper::sparse_container_ptr;
		using index_type = sparse_container::index_type;
		using index_iterator = IndexIterator;
		using functor = Functor;
	
		using typename base_type::difference_type;
		using typename base_type::reference;
	
	private:
		index_iterator m_index;
		sparse_container_ptr m_cont;
		functor m_func;
	
	private:
		void increment() noexcept { ++m_index; }
		void decrement() noexcept { --m_index; }
	
		void advance(difference_type n) noexcept { m_index += n; }
	
		auto dereference() const -> reference { auto idx = index(); return m_func(m_cont->get_item(idx), idx); }
		auto distance_to(const self_type & it) const noexcept -> difference_type { return it.m_index - m_index; }
		bool equal(const self_type & it) const noexcept { return m_cont == it.m_cont and m_index == it.m_index; }
	
	public:
		auto index() const noexcept { return helper::index(m_index); }
	
	public:
		base_sparse_container_transform_iterator(sparse_container_ref cont, index_iterator pos, functor func = {}) noexcept
			: m_cont(&cont), m_index(pos), m_func(func) {}

		// copy constructor from const iterator
		base_sparse_container_transform_iterator(const base_sparse_container_transform_iterator<functor, index_iterator, true> & it) noexcept
			: m_cont(it.m_cont), m_index(it.m_index), m_func(it.m_func) {}
	};


	template <class functor, class index_iterator> using const_sparse_container_transform_iterator = base_sparse_container_transform_iterator<functor, index_iterator, true>;
	template <class functor, class index_iterator> using       sparse_container_transform_iterator = base_sparse_container_transform_iterator<functor, index_iterator, false>;

	template <class functor, class index_iterator> using const_sparse_container_transform_range = boost::iterator_range<const_sparse_container_transform_iterator<functor, index_iterator>>;
	template <class functor, class index_iterator> using       sparse_container_transform_range = boost::iterator_range<sparse_container_transform_iterator<functor, index_iterator>>;

}
