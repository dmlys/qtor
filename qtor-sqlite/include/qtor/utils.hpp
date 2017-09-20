#pragma once
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/irange.hpp>

#include <qtor/types.hpp>
#include <qtor/sparse_container.hpp>
#include <ext/range/combine.hpp>
#include <ext/range/adaptors/pair.hpp>


namespace qtor
{
	/************************************************************************/
	/*               base_sparse_container_field_iterator                   */
	/************************************************************************/
	template <class index_iterator>
	struct sparse_container_helper
	{
		using iterator_category = typename std::iterator_traits<index_iterator>::iterator_category;
		static sparse_container::index_type index(const index_iterator & it) noexcept { return *it; }
	};

	template <>
	struct sparse_container_helper<sparse_container::index_type>
	{
		using iterator_category = std::random_access_iterator_tag;
		static sparse_container::index_type index(sparse_container::index_type idx) noexcept { return idx; }
	};


	/************************************************************************/
	/*                base_sparse_container_field_iterator                  */
	/************************************************************************/
	template <class IndexIterator, bool isconst>
	class base_sparse_container_field_iterator :
		public boost::iterator_facade<
			base_sparse_container_field_iterator<IndexIterator, isconst>,
			sparse_container::any_type,
			typename sparse_container_helper<IndexIterator>::iterator_category,
			std::conditional_t<isconst, const sparse_container::any_type &, sparse_container::any_type &>
		>
	{
		using self_type = base_sparse_container_field_iterator;
		using base_type = boost::iterator_facade<
			self_type,
			sparse_container::any_type, 
			typename sparse_container_helper<IndexIterator>::iterator_category,
			std::conditional_t<isconst, const sparse_container::any_type &, sparse_container::any_type &>
		>;
	
		friend boost::iterator_core_access;
		friend base_sparse_container_field_iterator<IndexIterator, true>;
	
	public:
		using container_ref = std::conditional_t<isconst, const sparse_container &, sparse_container &>;
		using container_ptr = std::conditional_t<isconst, const sparse_container *, sparse_container *>;
		using index_type = sparse_container::index_type;
		using index_iterator = IndexIterator;
	
		using typename base_type::difference_type;
		using typename base_type::reference;
	
	private:
		index_iterator m_index;
		container_ptr m_cont;
	
	private:
		void increment() noexcept { ++m_index; }
		void decrement() noexcept { --m_index; }
	
		void advance(difference_type n) noexcept { m_index += n; }
	
		auto dereference() const -> reference                                    { return m_cont->get_item(index()); }
		auto distance_to(const self_type & it) const noexcept -> difference_type { return it.m_index - m_index; }
		bool equal(const self_type & it) const noexcept { return m_cont == it.m_cont and m_index == it.m_index; }
	
	public:
		auto index() const noexcept { return sparse_container_helper<index_iterator>::index(m_index); }
	
	public:
		base_sparse_container_field_iterator(container_ref cont, index_iterator pos) noexcept
			: m_cont(&cont), m_index(pos) {}

		// copy constructor from const iterator
		base_sparse_container_field_iterator(const base_sparse_container_field_iterator<index_iterator, true> & it) noexcept
			: m_cont(it.m_cont), m_index(it.m_index) {}
	};
	
	template <class index_iterator> using const_sparse_container_field_iterator = base_sparse_container_field_iterator<index_iterator, true>;
	template <class index_iterator> using       sparse_container_field_iterator = base_sparse_container_field_iterator<index_iterator, false>;

	template <class index_iterator> using const_sparse_container_field_range = boost::iterator_range<const_sparse_container_field_iterator<index_iterator>>;
	template <class index_iterator> using       sparse_container_field_range = boost::iterator_range<      sparse_container_field_iterator<index_iterator>>;
	
	template <class index_range>
	auto make_field_range(sparse_container & cont, const index_range & idxs)
	{
		using index_iterator = typename boost::range_const_iterator<index_range>::type;
		return boost::make_iterator_range(
			sparse_container_field_iterator<index_iterator>(cont, boost::begin(idxs)),
			sparse_container_field_iterator<index_iterator>(cont, boost::end(idxs))
		);
	}

	template <class index_range>
	auto make_field_range(const sparse_container & cont, const index_range & idxs)
	{
		using index_iterator = typename boost::range_const_iterator<index_range>::type;
		return boost::make_iterator_range(
			const_sparse_container_field_iterator<index_iterator>(cont, boost::begin(idxs)),
			const_sparse_container_field_iterator<index_iterator>(cont, boost::end(idxs))
		);
	}

	inline auto make_field_range(sparse_container & cont)
	{
		using index_type = sparse_container::index_type;
		
		const index_type first = 0;
		const index_type last = static_cast<index_type>(cont.items().size());
		
		return boost::make_iterator_range(
			sparse_container_field_iterator<index_type>(cont, first),
			sparse_container_field_iterator<index_type>(cont, last)
		);
	}
	
	inline auto make_field_range(const sparse_container & cont)
	{
		using index_type = sparse_container::index_type;

		const index_type first = 0;
		const index_type last = static_cast<index_type>(cont.items().size());

		return boost::make_iterator_range(
			const_sparse_container_field_iterator<index_type>(cont, first),
			const_sparse_container_field_iterator<index_type>(cont, last)
		);
	}

	
	/************************************************************************/
	/*               sparse_container_field_string_iterator                 */
	/************************************************************************/
	template <class IndexIterator>
	class sparse_container_field_string_iterator :
		public boost::iterator_facade<
			sparse_container_field_string_iterator<IndexIterator>,
			string_type,
			typename sparse_container_helper<IndexIterator>::iterator_category,
			string_type
		>
	{
		using self_type = sparse_container_field_string_iterator;
		using base_type = boost::iterator_facade<
			self_type,
			string_type,
			typename sparse_container_helper<IndexIterator>::iterator_category,
			string_type
		>;
	
		friend boost::iterator_core_access;
	
	public:
		using typename base_type::reference;
		using typename base_type::difference_type;

	public:
		using index_iterator = IndexIterator;
		using index_type = sparse_container::index_type;
	
	private:
		index_iterator m_index;
		const sparse_container * m_cont;
		const sparse_container_meta * m_meta;
	
	private:
		reference dereference() const
		{
			auto idx = index();
			const auto & val = m_cont->get_item(idx);
			return QtTools::FromQString(m_meta->format_item(idx, val));
		}
	
		void increment() noexcept { ++m_index; }
		void decrement() noexcept { --m_index; }
	
		void advance(difference_type n) noexcept { m_index += n; }
		auto distance_to(const self_type & it) const noexcept -> difference_type { return it.m_index - m_index; }
		bool equal(const self_type & it) const noexcept { return m_cont == it.m_cont and m_index == it.m_index; }
	
	public:
		auto index() const noexcept { return sparse_container_helper<index_iterator>::index(m_index); }
	
	public:
		sparse_container_field_string_iterator(const sparse_container_meta & meta, const sparse_container & cont, index_iterator pos) noexcept
			: m_cont(&cont), m_index(pos), m_meta(&meta) {}
	};
	
	
	template <class index_iterator>
	using sparse_container_field_string_range = boost::iterator_range<sparse_container_field_string_iterator<index_iterator>>;


	template <class index_range>
	auto make_field_string_range(const sparse_container_meta & meta, const sparse_container & cont, const index_range & idxs)
	{
		using index_iterator = typename boost::range_const_iterator<index_range>::type;
		return boost::make_iterator_range(
			sparse_container_field_string_iterator<index_iterator>(meta, cont, boost::begin(idxs)),
			sparse_container_field_string_iterator<index_iterator>(meta, cont, boost::end(idxs))
		);
	}


	inline auto make_field_string_range(const sparse_container_meta & meta, const sparse_container & cont)
	{
		using index_type = sparse_container::index_type;

		const index_type first = 0;
		const index_type last = static_cast<index_type>(cont.items().size());

		return boost::make_iterator_range(
			sparse_container_field_string_iterator<index_type>(meta, cont, first),
			sparse_container_field_string_iterator<index_type>(meta, cont, last)
		);
	}

	
	
	/************************************************************************/
	/*                      torrents_batch_range                            */
	/************************************************************************/
	template <class NamesRange, class TorrentsRange>
	class torrents_batch_range :
		public ext::input_range_facade<
			torrents_batch_range<NamesRange, TorrentsRange>,
			ext::combined_range<NamesRange, sparse_container_field_string_range<sparse_container::index_type>>,
			ext::combined_range<NamesRange, sparse_container_field_string_range<sparse_container::index_type>>
		>
	{
		using self_type = torrents_batch_range;
		using base_type = ext::input_range_facade<
			self_type, 
			ext::combined_range<NamesRange, sparse_container_field_string_range<sparse_container::index_type>>,
			ext::combined_range<NamesRange, sparse_container_field_string_range<sparse_container::index_type>>
		>;
	
	public:
		using typename base_type::reference;
		using names_range = NamesRange;
		using torrents_range = TorrentsRange;
	
	private:
		using torrents_iterator = typename torrents_range::const_iterator;
	
	private:
		const names_range * m_names;
		const torrents_range * m_torrents;
		const sparse_container_meta * m_meta;
		mutable torrents_iterator m_cur, m_last;
	
	public:
		bool empty() const noexcept { return m_cur != m_last; }
		void pop_front() const { ++m_cur; }
		reference front() const { return ext::combine(*m_names, make_field_string_range(*m_meta, *m_cur)); }
	
	public:
		torrents_batch_range(const sparse_container_meta & meta, 
			const names_range & names, const torrents_range & torrents)
		: m_meta(&meta), m_names(&names), m_torrents(&torrents),
		  m_cur(torrents.begin()), m_last(torrents.end())
		{ }
	};
	
	template <class NamesRange, class TorrentsRange>
	auto make_batch_range(const sparse_container_meta & meta, const NamesRange & names, const TorrentsRange & torrents)
	{
		return torrents_batch_range<NamesRange, TorrentsRange>(meta, names, torrents);
	}
}
