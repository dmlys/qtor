#pragma once
#include <qtor/types.hpp>
#include <qtor/field_iterator.hpp>

#include <ext/range/combine.hpp>
#include <ext/range/input_range_facade.hpp>
#include <boost/iterator/iterator_adaptor.hpp>

namespace qtor
{
	using types_variant = variant<
		uint64_type,
		bool,
		double,
		string_type,
		datetime_type,
		duration_type, 
		nullopt_t
	>;

	class make_types_variant
	{
	public:
		using any_type   = model_meta::any_type;
		using index_type = model_meta::index_type;

	private:
		const model_meta * m_meta;

	public:
		using result_type = types_variant;
		types_variant operator()(const any_type & val, index_type index) const;

	public:
		make_types_variant(const model_meta & meta)
			: m_meta(&meta) {}
	};


	template <class Type, class IndexIterator>
	class types_variant_iterator :
		public boost::iterator_adaptor<
			/* Derived    */ types_variant_iterator<Type, IndexIterator>,
			/* Base       */ const_field_iterator<Type, IndexIterator>,
	        /* value_type */ types_variant,
	        /* category   */ typename const_field_iterator<Type, IndexIterator>::iterator_category,
	        /* reference  */ types_variant
		>
	{
		friend boost::iterator_core_access;

		using self_type = types_variant_iterator;
		using base_type = boost::iterator_adaptor<
			/* Derived    */ types_variant_iterator<Type, IndexIterator>,
			/* Base       */ const_field_iterator<Type, IndexIterator>,
	        /* value_type */ types_variant,
			/* category   */ typename const_field_iterator<Type, IndexIterator>::iterator_category,
	        /* reference  */ types_variant
		>;

	public:
		using typename base_type::reference;

	private:
		reference dereference() const;

	public:
		using base_type::base_type;
	};

	template <class Type, class IndexIterator>
	auto types_variant_iterator<Type, IndexIterator>::dereference() const -> reference
	{
		const auto & base_it = *this->base();
		const auto * meta = base_it.meta();
		const auto idx = base_it.index();

		const auto val = *base_it;
		return make_types_variant(*meta)(val, idx);
	}

	template <class Type>
	using types_variant_range = boost::iterator_range<types_variant_iterator<Type, model_meta::index_type>>;


	template <class Type>
	inline auto make_types_variant_range(const model_accessor<Type> & meta, const Type & val)
	{
		using types_iterator = types_variant_iterator<Type, model_meta::index_type>;
		using field_iterator = const_field_iterator<Type, model_meta::index_type>;
		auto first = static_cast<model_meta::index_type>(0);
		auto last = static_cast<model_meta::index_type>(meta.item_count());

		return boost::make_iterator_range(
			types_iterator(field_iterator(&meta, val, first)),
			types_iterator(field_iterator(&meta, val, last))
		);
	}
	
	/************************************************************************/
	/*                      torrents_batch_range                            */
	/************************************************************************/
	template <class NamesRange, class ValuesRange>
	class batch_range :
		public ext::input_range_facade<
			batch_range<NamesRange, ValuesRange>,
			ext::combined_range<NamesRange, types_variant_range<ext::range_value_t<ValuesRange>>>,
			ext::combined_range<NamesRange, types_variant_range<ext::range_value_t<ValuesRange>>>
		>
	{
		using self_type = batch_range;
		using base_type = ext::input_range_facade<
			self_type, 
			ext::combined_range<NamesRange, types_variant_range<ext::range_value_t<ValuesRange>>>,
			ext::combined_range<NamesRange, types_variant_range<ext::range_value_t<ValuesRange>>>
		>;		

	public:
		using typename base_type::value_type;
		using typename base_type::reference;
		using names_range = NamesRange;
		using values_range = ValuesRange;
	
	private:
		using values_iterator = typename values_range::const_iterator;
	
	private:
		const names_range * m_names;
		const values_range * m_torrents;
		const model_accessor<value_type> * m_meta;
		mutable values_iterator m_cur, m_last;
	
	public:
		bool empty() const noexcept { return m_cur == m_last; }
		void pop_front() const { ++m_cur; }
		reference front() const { return ext::combine(*m_names, make_types_variant_range(*m_meta, *m_cur)); }
	
	public:
		batch_range(const model_accessor<value_type> & meta, const names_range & names, const values_range & torrents)
			: m_names(&names), m_torrents(&torrents), m_meta(&meta),
			  m_cur(torrents.begin()), m_last(torrents.end())
		{ }
	};
	
	template <class Type, class NamesRange, class ValuesRange>
	auto make_batch_range(const model_accessor<Type> & meta, const NamesRange & names, const ValuesRange & values)
	{
		using range_type = ext::range_value_t<ValuesRange>;
		static_assert(std::is_convertible_v<range_type, Type>);
		return batch_range<NamesRange, ValuesRange>(meta, names, values);
	}
}
