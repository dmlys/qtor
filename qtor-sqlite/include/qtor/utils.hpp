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

	types_variant make_types_variant(const model_meta * meta, const model_meta::any_type & val, model_meta::index_type index);




	template <class type, class index_iterator>
	class types_variant_iterator :
		public boost::iterator_adaptor<
			/* Derived    */ types_variant_iterator<type, index_iterator>,
			/* Base       */ const_field_iterator<type, index_iterator>,
	        /* value_type */ types_variant,
	        /* category   */ typename const_field_iterator<type, index_iterator>::iterator_category,
	        /* reference  */ types_variant
		>
	{
		friend boost::iterator_core_access;

		using self_type = types_variant_iterator;
		using base_type = boost::iterator_adaptor<
			/* Derived    */ types_variant_iterator<type, index_iterator>,
			/* Base       */ const_field_iterator<type, index_iterator>,
	        /* value_type */ types_variant,
			/* category   */ typename const_field_iterator<type, index_iterator>::iterator_category,
	        /* reference  */ types_variant
		>;

	private:
		auto dereference() const;

	public:
		using base_type::base_type;

		types_variant_iterator(const model_accessor<type> * meta, const type & valref, index_iterator idx)
		    : base_type(typename base_type::base_type(meta, valref, idx)) {}
	};

	template <class type, class index_iterator>
	auto types_variant_iterator<type, index_iterator>::dereference() const
	{
		const auto & base_it = this->base_reference();
		const auto * meta = base_it.meta();
		const auto idx = base_it.index();

		const auto val = *base_it;
		return make_types_variant(meta, val, idx);
	}

	template <class type>
	using types_variant_range = boost::iterator_range<types_variant_iterator<type, model_meta::index_type>>;


	template <class Type>
	inline auto make_types_variant_range(const model_accessor<Type> & meta, const Type & val)
	{
		auto first = static_cast<model_meta::index_type>(0);
		auto last = static_cast<model_meta::index_type>(meta.item_count());

		return boost::make_iterator_range(
			types_variant_iterator(&meta, val, first),
			types_variant_iterator(&meta, val, last)
		);
	}
	
	/************************************************************************/
	/*                          batch_range                                 */
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
		const values_range * m_values;
		const model_accessor<ext::range_value_t<ValuesRange>> * m_meta;
		mutable values_iterator m_cur, m_last;
	
	public:
		bool empty() const noexcept { return m_cur == m_last; }
		void pop_front() const { ++m_cur; }
		reference front() const { return ext::combine(*m_names, make_types_variant_range(*m_meta, *m_cur)); }
	
	public:
		batch_range(const model_accessor<ext::range_value_t<ValuesRange>> & meta, const names_range & names, const values_range & values)
			: m_names(&names), m_values(&values), m_meta(&meta),
			  m_cur(values.begin()), m_last(values.end())
		{ }
	};
	
	template <class Type, class NamesRange, class ValuesRange>
	auto make_batch_range(const model_accessor<Type> & meta, const NamesRange & names, const ValuesRange & values)
	{
		static_assert(ext::is_range_of_v<ValuesRange, Type>);
		return batch_range(meta, names, values);
	}
}
