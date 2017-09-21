#pragma once
#include <qtor/types.hpp>
#include <qtor/sparse_container.hpp>
#include <qtor/sparse_container_iterator.hpp>

#include <ext/range/combine.hpp>
#include <ext/range/input_range_facade.hpp>


namespace qtor
{
	using sparse_variant = variant<
		uint64_type,
		bool,
		double,
		string_type,
		datetime_type,
		duration_type, 
		nullopt_t
	>;

	class to_sparse_variant
	{
	public:
		using any_type = sparse_container::any_type;
		using index_type = sparse_container::index_type;

	private:
		const sparse_container_meta * m_meta;

	public:
		sparse_variant operator()(const any_type & val, index_type index) const;

	public:
		to_sparse_variant(const sparse_container_meta & meta) 
			: m_meta(&meta) {}
	};



	using sparse_container_variant_iterator = const_sparse_container_transform_iterator<
		to_sparse_variant,
		sparse_container::index_type
	>;

	using sparse_container_variant_range = const_sparse_container_transform_range<
		to_sparse_variant,
		sparse_container::index_type
	>;

	inline auto make_sparse_container_variant_range(const sparse_container_meta & meta, const sparse_container & cont)
	{
		to_sparse_variant func {meta};
		auto first = static_cast<sparse_container::index_type>(0);
		auto last = static_cast<sparse_container::index_type>(meta.items_count());

		return boost::make_iterator_range(
			sparse_container_variant_iterator(cont, first, func),
			sparse_container_variant_iterator(cont, last, func)
		);
	}
	
	/************************************************************************/
	/*                      torrents_batch_range                            */
	/************************************************************************/
	template <class NamesRange, class TorrentsRange>
	class torrents_batch_range :
		public ext::input_range_facade<
			torrents_batch_range<NamesRange, TorrentsRange>,
			ext::combined_range<NamesRange, sparse_container_variant_range>,
			ext::combined_range<NamesRange, sparse_container_variant_range>
		>
	{
		using self_type = torrents_batch_range;
		using base_type = ext::input_range_facade<
			self_type, 
			ext::combined_range<NamesRange, sparse_container_variant_range>,
			ext::combined_range<NamesRange, sparse_container_variant_range>
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
		bool empty() const noexcept { return m_cur == m_last; }
		void pop_front() const { ++m_cur; }
		reference front() const { return ext::combine(*m_names, make_sparse_container_variant_range(*m_meta, *m_cur)); }
	
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
