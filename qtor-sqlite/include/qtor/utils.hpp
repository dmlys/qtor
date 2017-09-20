#pragma once
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>

#include <qtor/types.hpp>
#include <qtor/sparse_container.hpp>
#include <ext/range/combine.hpp>
#include <ext/range/adaptors/pair.hpp>


namespace qtor
{
	/************************************************************************/
	/*               base_sparse_container_field_iterator                   */
	/************************************************************************/
	template <bool isconst>
	class base_sparse_container_field_iterator :
		public boost::iterator_facade<
			base_sparse_container_field_iterator<isconst>,
			sparse_container::any_type,
			std::random_access_iterator_tag,
			std::conditional_t<isconst, const sparse_container::any_type &, sparse_container::any_type &>
		>
	{
		using self_type = base_sparse_container_field_iterator;
		using base_type = boost::iterator_facade<
			self_type,
			sparse_container::any_type, 
			std::random_access_iterator_tag,
			std::conditional_t<isconst, const sparse_container::any_type &, sparse_container::any_type &>
		>;
	
		friend boost::iterator_core_access;
		friend base_sparse_container_field_iterator<true>;
	
	public:
		using container_ref = std::conditional_t<isconst, const sparse_container &, sparse_container &>;
		using container_ptr = std::conditional_t<isconst, const sparse_container *, sparse_container *>;
		using index_type = sparse_container::index_type;
	
		using typename base_type::difference_type;
		using typename base_type::reference;
	
	private:
		index_type m_index;
		container_ptr m_cont;
	
	private:
		void increment() noexcept { ++m_index; }
		void decrement() noexcept { --m_index; }
	
		void advance(difference_type n) noexcept { m_index += n; }
	
		auto dereference() const -> reference                                    { return m_cont->get_item(m_index); }
		auto distance_to(const self_type & it) const noexcept -> difference_type { return it.m_index - m_index; }
		bool equal(const self_type & it) const noexcept { return m_cont == it.m_cont and m_index == it.m_index; }
	
	public:
		auto index() const noexcept { return m_index; }
	
	public:
		base_sparse_container_field_iterator(container_ref cont, index_type pos) noexcept
			: m_cont(&cont), m_index(pos) {}

		// copy constructor from const iterator
		base_sparse_container_field_iterator(const base_sparse_container_field_iterator<true> & it) noexcept
			: m_cont(it.m_cont), m_index(it.m_index) {}
	};
	
	typedef base_sparse_container_field_iterator<true>  const_sparse_container_field_iterator;
	typedef base_sparse_container_field_iterator<false>       sparse_container_field_iterator;

	using sparse_container_field_range = boost::iterator_range<sparse_container_field_iterator>;
	using const_sparse_container_field_range = boost::iterator_range<const_sparse_container_field_iterator>;
	

	auto make_field_range(sparse_container & cont)
	{
		auto count = static_cast<sparse_container::index_type>(cont.items().size());
		return boost::make_iterator_range(
			sparse_container_field_iterator(cont, 0),
			sparse_container_field_iterator(cont, count)
		);
	}
	
	auto make_field_range(const sparse_container & cont)
	{
		auto count = static_cast<sparse_container::index_type>(cont.items().size());
		return boost::make_iterator_range(
			const_sparse_container_field_iterator(cont, 0),
			const_sparse_container_field_iterator(cont, count)
		);
	}

	
	/************************************************************************/
	/*               sparse_container_field_string_iterator                 */
	/************************************************************************/
	class sparse_container_field_string_iterator :
		public boost::iterator_facade<
			sparse_container_field_string_iterator,
			string_type,
			std::random_access_iterator_tag,
			string_type
		>
	{
		using self_type = sparse_container_field_string_iterator;
		using base_type = boost::iterator_facade<
			self_type,
			string_type,
			boost::use_default,
			string_type
		>;
	
		friend boost::iterator_core_access;
	
	public:
		using index_type = sparse_container::index_type;
	
	private:
		sparse_container::index_type m_index;
		const sparse_container * m_cont;
		const sparse_container_meta * m_meta;
	
	private:
		reference dereference() const
		{
			const auto & val = m_cont->get_item(m_index);
			return QtTools::FromQString(m_meta->format_item(m_index, val));
		}
	
		void increment() noexcept { ++m_index; }
		void decrement() noexcept { --m_index; }
	
		void advance(difference_type n) noexcept { m_index += n; }
		auto distance_to(const self_type & it) const noexcept -> difference_type { return it.m_index - m_index; }
		bool equal(const self_type & it) const noexcept { return m_cont == it.m_cont and m_index == it.m_index; }
	
	public:
		auto index() const noexcept { return m_index; }
	
	public:
		sparse_container_field_string_iterator(const sparse_container_meta & meta, const sparse_container & cont, index_type pos) noexcept
			: m_cont(&cont), m_index(pos), m_meta(&meta) {}
	};
	
	
	using sparse_container_field_string_range = boost::iterator_range<sparse_container_field_string_iterator>;


	auto make_field_string_range(const sparse_container_meta & meta, const sparse_container & cont)
	{
		auto count = static_cast<sparse_container::index_type>(cont.items().size());
		return boost::make_iterator_range(
			sparse_container_field_string_iterator(meta, cont, 0),
			sparse_container_field_string_iterator(meta, cont, count)
		);
	}

	
	
	/************************************************************************/
	/*                      torrents_batch_range                            */
	/************************************************************************/
	template <class NamesRange, class TorrentsRange>
	class torrents_batch_range :
		public ext::input_range_facade<
			torrents_batch_range<NamesRange, TorrentsRange>,
			ext::combined_range<NamesRange, sparse_container_field_string_range>,
			ext::combined_range<NamesRange, sparse_container_field_string_range>
		>
	{
		using self_type = torrents_batch_range;
		using base_type = ext::input_range_facade<
			self_type, 
			ext::combined_range<NamesRange, sparse_container_field_string_range>,
			ext::combined_range<NamesRange, sparse_container_field_string_range>
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
