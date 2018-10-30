#pragma once
#include <memory>
#include <utility>
#include <stdexcept>
#include <ext/type_traits.hpp>

#include <boost/predef.h> // for workaround, see end of this file
#include <boost/config.hpp>
#include <boost/static_assert.hpp>
#include <boost/any.hpp>

namespace any_filter_detail
{
	/************************************************************************/
	/*             has_set_expr                                             */
	/************************************************************************/
	template <class FilterType>
	struct set_expr_exists_test
	{
		template <class FilterType, class = decltype(std::declval<FilterType>().set_expr(std::declval<const boost::any &>()))>
		static ext::detail::Yes test(int);

		template <class Type>
		static ext::detail::No test(...);
		
		static const bool value = sizeof(test<FilterType>(0)) == sizeof(ext::detail::Yes);
	};

	template <class FilterType, bool = set_expr_exists_test<FilterType>::value>
	struct set_expr_test : std::false_type {};

	template <class FilterType>
	struct set_expr_test<FilterType, true> : 
		std::is_convertible
		<
			decltype(std::declval<FilterType>().set_expr(std::declval<const boost::any &>())),
			std::pair<bool, bool>
		>
	{};

	template <class FilterType>
	struct has_set_expr : set_expr_test<std::decay_t<FilterType>> {};

	/************************************************************************/
	/*                      has_matches                                     */
	/************************************************************************/
	template <class FilterType, class Type>
	struct matches_exists_test
	{
		template <
			class FilterType, class Type,
			class = decltype(std::declval<FilterType>().matches(std::declval<const Type &>()))
		>
		static ext::detail::Yes test(int);

		template <class FilterType, class Type>
		static ext::detail::No test(...);

		static const bool value = sizeof(test<FilterType, Type>(0)) == sizeof(ext::detail::Yes);
	};

	template <class FilterType, class Type, bool = matches_exists_test<FilterType, Type>::value>
	struct matches_test : std::false_type {};

	template <class FilterType, class Type>
	struct matches_test<FilterType, Type, true> :
		std::is_same
		<
			decltype(std::declval<FilterType>().matches(std::declval<const Type &>())),
			bool
		>
	{};

	template <class FilterType, class Type>
	struct has_matches : matches_test<std::decay_t<FilterType>, std::decay_t<Type>> {};

	/************************************************************************/
	/*                 always_matches_test                                  */
	/************************************************************************/
	template <class FilterType>
	struct always_matches_exists_test
	{
		template <class FilterType, class = decltype(std::declval<FilterType>().always_matches())>
		static ext::detail::Yes test(int);

		template <class FilterType>
		static ext::detail::No test(...);

		static const bool value = sizeof(test<FilterType>(0)) == sizeof(ext::detail::Yes);
	};

	template <class FilterType, bool = always_matches_exists_test<FilterType>::value>
	struct always_matches_test : std::false_type {};

	template <class FilterType>
	struct always_matches_test<FilterType, true> :
		std::is_same
		<
			decltype(std::declval<FilterType>().always_matches()),
			bool
		>
	{ };

	template <class FilterType>
	struct has_always_matches : always_matches_test<std::decay_t<FilterType>> {};

}

/// Type erased filter. Можно было бы попробовать boost::type_erasure.
/// пока ручная реализация + можем сделать некоторую кастомизацию, как, например, опциональный метод set_expr
/// 
/// Фильтр должен реализовывать следующий интерфейс:
///   bool always_matches() const - возвращает что фильтр всегда пропускает все элементы, например, в случае если фильтр пуст
///   bool matches(const Type & val) const - вычисляет удовлетворяет ли данное значение критерию фильтрации
/// необязательные методы:
///   std::pair<bool, bool> set_expr(const boost::any &)
///     устанавливает новые параметры фильтрации, поскольку для каждого фильтра они специфичны и
///     могут полностью отличаться - передаем через boost::any.
///     возвращает изменился ли критерий фильтрации, если true,
///     то второй bool указывает можно ли выполнить инкрементальный поиск
///     
///     в случае передачи данных не поддерживаемого типа - фильтр действует по своему усмотрению,
///     может проигнорировать или бросить исключение(в том чилсе boost::bad_any_cast)
///     в случае, если фильтр не рализует данный метод - по умолчанию бросается std::logic_error
/// 
/// @Param Type тип фильтруемых элементов
template <class Type>
class any_filter
{
public:
	typedef Type value_type;
	/// смотри описание метода set_expr
	typedef std::pair<bool, bool> expr_result;

private:
	struct filter_base
	{
		virtual ~filter_base() = default;
		virtual bool always_matches() const = 0;
		virtual bool matches(const value_type & val) const = 0;
		virtual expr_result set_expr(const boost::any & expr) = 0;
	};


	template <class FilterType>
	struct filter_impl : filter_base
	{
		FilterType m_filter;

	public:
		filter_impl(FilterType filter) : m_filter(std::move(filter)) {}

	public:
		template <class Filter, typename std::enable_if<any_filter_detail::has_set_expr<Filter>::value, int>::type = 0>
		inline static std::pair<bool, bool> set_expr(Filter & filter, const boost::any & a)
		{
			return filter.set_expr(a);
		}

		template <class Filter, typename std::enable_if<not any_filter_detail::has_set_expr<Filter>::value, int>::type = 0>
		BOOST_NORETURN
		inline static std::pair<bool, bool> set_expr(Filter & filter, const boost::any & a)
		{
			throw std::logic_error("filter does not support set_expr");
		}

		expr_result set_expr(const boost::any & a) override
		{
			return filter_impl::set_expr(m_filter, a);
		}

		bool always_matches()                const override { return m_filter.always_matches(); }
		bool matches(const value_type & val) const override { return m_filter.matches(val); }
	};

private:
	bool m_nofilter = true;
	std::unique_ptr<filter_base> m_filter;
	boost::typeindex::type_index m_type = boost::typeindex::type_id<void>();

public:
	template <class Filter>
	void assign(Filter && filter)
	{
		using namespace any_filter_detail;
		BOOST_STATIC_ASSERT(has_matches<Filter, value_type>::value);
		BOOST_STATIC_ASSERT(has_always_matches<Filter>::value);

		m_type = boost::typeindex::type_id<Filter>();
		m_filter = std::make_unique<filter_impl<Filter>>(std::forward<Filter>(filter));
		m_nofilter = m_filter->always_matches();
	}

	bool empty() const
	{
		return static_cast<bool>(m_filter);
	}

	/// удаляет фильтр, приводя контейнер в default construct состояние
	void clear()
	{
		m_type = boost::typeindex::type_id<void>();
		m_filter = nullptr;
		m_nofilter = true;
	}

	const boost::typeindex::type_info & type() const
	{
		return m_type.type_info();
	}

public:
	inline bool matches(const Type & rec) const { return m_filter->matches(rec); }
	inline bool always_matches() const { return m_nofilter; }
	
	/// same as matches
	inline bool operator()(const Type & rec) const { return matches(rec); }
	/// same as always_matches
	inline explicit operator bool() const { return !always_matches(); }
	
	std::pair<bool, bool> set_expr(const boost::any & expr)
	{
		auto ret = m_filter->set_expr(expr);
		m_nofilter = m_filter->always_matches();
		return ret;
	};

public:
	any_filter() = default;
	~any_filter() = default;

	any_filter(const any_filter &) = delete;
	any_filter & operator =(const any_filter &) = delete;

	template <class Filter>
	any_filter(Filter && filter)
	{
		assign(std::forward<Filter>(filter));
	}

	template <class Filter>
	any_filter & operator =(Filter && filter)
	{
		assign(std::forward<Filter>(filter));
		return *this;
	}

	any_filter(any_filter && op)
		: m_filter(std::move(op.m_filter)),
		  m_nofilter(std::move(op.m_nofilter))
	{ }

	any_filter & operator =(any_filter && op)
	{
		if (this != &op)
		{
			m_filter = std::move(op.m_filter);
			m_nofilter = std::move(op.m_nofilter);
		}

		return *this;
	}

	//template <class Type, class FilterType>
	//friend Type * any_cast(AnyFilter<FilterType> * op) BOOST_NOEXCEPT;

	template <class Type, class FilterType>
	friend Type * unsafe_any_cast(any_filter<FilterType> * op) BOOST_NOEXCEPT;
};

/************************************************************************/
/*                      casts                                           */
/************************************************************************/
template <class ValueType, class FilterType>
inline ValueType * unsafe_any_cast(any_filter<FilterType> * op)
{
	typedef typename any_filter<FilterType>::template filter_impl<ValueType> Impl;
	return &static_cast<Impl *>(op->m_filter.get())->m_filter;
}

template <class ValueType, class FilterType>
inline const ValueType * unsafe_any_cast(const any_filter<FilterType> * op)
{
	return unsafe_any_cast<ValueType>(const_cast<any_filter<FilterType> *>(op));
}

template <class ValueType, class FilterType>
ValueType * any_cast(any_filter<FilterType> * op)
{
	return !op->empty() && op->type() == boost::typeindex::type_id<ValueType>()
		? unsafe_any_cast<ValueType>(op) : nullptr;
}

template <class ValueType, class FilterType>
const ValueType * any_cast(const any_filter<FilterType> * op)
{
	return !op->empty() && op->type() == boost::typeindex::type_id<ValueType>()
		? unsafe_any_cast<ValueType>(op) : nullptr;
}

template <class ValueType, class FilterType>
ValueType any_cast(any_filter<FilterType> & op)
{
	typedef typename std::decay<ValueType>::type nonref;
	if (op.empty() || boost::typeindex::type_id<nonref>() != op.type())
		boost::throw_exception(boost::bad_any_cast());

	return *unsafe_any_cast<nonref>(&op);
}

template <class ValueType, class FilterType>
inline ValueType any_cast(const any_filter<FilterType> & op)
{
	return any_cast<ValueType>(const_cast<any_filter<FilterType> &>(op));
}

#if BOOST_COMP_MSVC
/// MSVC фигово проверяет поддержку копирования и по факту всегда выдает true
/// в качестве work-around'а явно специализируем
template <class Type>
struct std::is_copy_constructible<any_filter<Type>> : std::false_type {};
#endif

