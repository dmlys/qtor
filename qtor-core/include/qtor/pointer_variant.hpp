#pragma once
#include <cstddef>
#include <memory>  // for std::unique_ptr
#include <utility> // for std::exchange
#include <type_traits>
#include <ext/type_traits.hpp>

#include <boost/integer/static_log2.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/bind.hpp>

namespace viewed
{
	/// pointer_variant<int *, type *, ...>
	/// variant class for pointer types(only pointer types allowed).
	/// sizeof(pointer_variant<...>) == sizeof(std::uintptr_t) always,
	/// current type is held in lowest bits of pointer value.
	/// 
	/// Only head allocated pointers or pointers with big enough alignment are allowed.
	/// (only runtime assert checks are made, so you should be careful)
	template <class ... PointerTypes>
	class pointer_variant;

	/************************************************************************/
	/*                      pointer_variant_size                            */
	/************************************************************************/
	template <class Type>
	struct pointer_variant_size; /* undefined */
		
	template <class Type> struct pointer_variant_size<         const Type> : pointer_variant<Type>::type {};
	template <class Type> struct pointer_variant_size<      volatile Type> : pointer_variant<Type>::type {};
	template <class Type> struct pointer_variant_size<const volatile Type> : pointer_variant<Type>::type {};

	template <class... PointerTypes>
	struct pointer_variant_size<pointer_variant<PointerTypes...>>
		: std::integral_constant<std::size_t, sizeof...(PointerTypes)> {};

	template <class... PointerTypes>
	constexpr auto pointer_variant_size_v = pointer_variant_size<PointerTypes...>::value;


	template <class Type>
	struct is_pointer_variant : std::false_type {};

	template <class ... Types>
	struct is_pointer_variant<pointer_variant<Types...>> : std::true_type {};

	template <class Type>
	constexpr bool is_pointer_variant_v = is_pointer_variant<Type>::value;

	/************************************************************************/
	/*                   pointer_variant_alternative                        */
	/************************************************************************/
	template <std::size_t I, class Type>
	struct pointer_variant_alternative; // undefined

	template <size_t I, class Type>
	struct pointer_variant_alternative<I, const Type>
	{
		using type = std::add_const_t<typename pointer_variant_alternative<I, Type>::type>;
	};

	template <size_t I, class Type>
	struct pointer_variant_alternative<I, volatile Type>
	{
		using type = std::add_volatile_t<typename pointer_variant_alternative<I, Type>::type>;
	};

	template <size_t I, class Type>
	struct pointer_variant_alternative<I, const volatile Type>
	{
		using type = std::add_cv_t<typename pointer_variant_alternative<I, Type>::type>;
	};

	template <size_t I, class... PointerTypes>
	struct pointer_variant_alternative<I, pointer_variant<PointerTypes...>>
	{
		using type = boost::mp11::mp_at_c<pointer_variant<PointerTypes...>, I>;
	};

	template <size_t I, class Type>
	using pointer_variant_alternative_t = typename pointer_variant_alternative<I, Type>::type;

	/************************************************************************/
	/*                   forward declarations                               */
	/************************************************************************/
	template <class T, class ... PointerTypes>
	constexpr bool holds_alternative(const pointer_variant<PointerTypes...> & v) noexcept;

	template <class T, class ... Types>
	constexpr T get(const pointer_variant<Types...> & v);

	template <std::size_t Index, class ... Types>
	constexpr auto get(const pointer_variant<Types...> & v) ->
		pointer_variant_alternative_t<Index, pointer_variant<Types...>>;

	template <class Visitor, class ... Variants>
	constexpr auto visit(Visitor && vis, Variants && ... vars) ->
		std::enable_if_t<
			std::conjunction_v<is_pointer_variant<ext::remove_cvref_t<Variants>>...>,
			std::invoke_result_t<Visitor, boost::mp11::mp_first<ext::remove_cvref_t<Variants>>...>
		>;

	/************************************************************************/
	/*                    pointer_variant                                   */
	/************************************************************************/
	template <class ... PointerTypes>
	class pointer_variant
	{
		using self_type = pointer_variant;

		static constexpr unsigned TYPE_BITS = boost::static_log2<sizeof...(PointerTypes)>::value + (sizeof...(PointerTypes) % 2 != 0);
		static constexpr unsigned PTR_OFFSET = TYPE_BITS + 1;
		static constexpr std::uintptr_t PTR_MASK = ~std::uintptr_t(0) >> (sizeof(std::uintptr_t) * CHAR_BIT - PTR_OFFSET);

		static_assert(PTR_OFFSET <= boost::static_log2<sizeof(std::max_align_t)>::value,
			"not enough align bytes to store pointer type");

		static_assert(boost::mp11::mp_all_of<self_type, std::is_pointer>::value,
			"only pointer types allowed");

	private:
		union 
		{
			struct
			{
				std::uintptr_t m_owning : 1;
				std::uintptr_t m_type   : TYPE_BITS;
				std::uintptr_t m_ptr    : sizeof(std::uintptr_t) * CHAR_BIT - PTR_OFFSET;
			};

			std::uintptr_t m_val;
		};

	private:
		static constexpr std::uintptr_t pack(const void * extptr) { return reinterpret_cast<std::uintptr_t>(extptr) >> PTR_OFFSET; }
		static constexpr void * unpack(std::uintptr_t extptr)     { return reinterpret_cast<void *>(extptr << PTR_OFFSET); }

	private:
		void destroy() noexcept;

	public:
		constexpr void * pointer() const noexcept { return unpack(m_ptr); }
		constexpr std::size_t index() const noexcept { return m_type; }
		void swap(pointer_variant & other) noexcept { std::swap(m_val, other.m_val); }

	public:
		template <class Type> pointer_variant(Type * ptr) noexcept;
		template <class Type> pointer_variant & operator =(Type * ptr) noexcept;

		template <class Type> pointer_variant(std::unique_ptr<Type> ptr) noexcept;
		template <class Type> pointer_variant & operator =(std::unique_ptr<Type> ptr) noexcept;

	public:
		pointer_variant(pointer_variant &&) noexcept;
		pointer_variant & operator =(pointer_variant &&) noexcept;

		pointer_variant(const pointer_variant &) = delete;
		pointer_variant & operator =(const pointer_variant &) = delete;

		pointer_variant() noexcept;
		~pointer_variant() noexcept;
	};

	/************************************************************************/
	/*            pointer_variant implementation                            */
	/************************************************************************/
	template <class ... Types>
	inline void swap(pointer_variant<Types...> & v1, pointer_variant<Types...> & v2)
	{
		v1.swap(v2);
	}

	namespace pointer_variant_detail
	{
		template <class current, class searched> 
		using is_same = boost::mp11::mp_or<
			std::is_same<current, searched>,
			std::is_same<std::remove_cv_t<std::remove_pointer_t<current>>, std::remove_pointer_t<searched>>
		>;

		template <class list, class ptr_type>
		constexpr auto find_v = boost::mp11::mp_find_if_q<list, boost::mp11::mp_bind<is_same, boost::mp11::_1, ptr_type>>::value;
	}

	template <class ... PointerTypes>
	template <class Type>
	inline pointer_variant<PointerTypes...>::pointer_variant(Type * ptr) noexcept
	{
		constexpr auto idx = pointer_variant_detail::find_v<self_type, Type *>;
		static_assert(idx < boost::mp11::mp_size<self_type>::value, "pointer type not from given type list");
		assert((reinterpret_cast<std::uintptr_t>(ptr) & PTR_MASK) == 0);

		m_owning = 0;
		m_type = idx;
		m_ptr = pack(ptr);
	}

	template <class ... PointerTypes>
	template <class Type>
	inline auto pointer_variant<PointerTypes...>::operator =(Type * ptr) noexcept -> pointer_variant &	
	{
		constexpr auto idx = pointer_variant_detail::find_v<self_type, Type *>;
		static_assert(idx < boost::mp11::mp_size<self_type>::value, "pointer type not from given type list");
		assert((reinterpret_cast<std::uintptr_t>(ptr) & PTR_MASK) == 0);

		destroy();

		m_owning = 0;
		m_type = idx;
		m_ptr = pack(ptr);

		return *this;
	}

	template <class ... PointerTypes>
	template <class Type>
	inline pointer_variant<PointerTypes...>::pointer_variant(std::unique_ptr<Type> ptr) noexcept
		: pointer_variant(ptr.release())
	{
		m_owning = 1;
	}

	template <class ... PointerTypes>
	template <class Type>
	inline auto pointer_variant<PointerTypes...>::operator =(std::unique_ptr<Type> ptr) noexcept -> pointer_variant &	
	{
		m_owning = 1;
		return operator =(ptr.release());
	}

	template <class ... PointerTypes>	
	inline pointer_variant<PointerTypes...>::pointer_variant() noexcept
	{
		m_val = 0;
	}

	template <class ... PointerTypes>
	inline void pointer_variant<PointerTypes...>::destroy() noexcept
	{
		if (m_owning)
			visit([](auto * ptr) { delete ptr; }, *this);
	}

	template <class ... PointerTypes>
	inline pointer_variant<PointerTypes...>::~pointer_variant() noexcept
	{
		destroy();
	}

	template <class ... PointerTypes>
	inline pointer_variant<PointerTypes...>::pointer_variant(pointer_variant && op) noexcept	
	{
		m_val = std::exchange(op.m_val, 0);
	}

	template <class ... PointerTypes>
	inline auto pointer_variant<PointerTypes...>::operator =(pointer_variant && op) noexcept -> pointer_variant &
	{
		if (this != &op)
		{
			std::destroy_at(this);
			new(this) pointer_variant(std::move(op));
		}

		return *this;
	}


	template <class T, class ... PointerTypes>
	constexpr inline bool holds_alternative(const pointer_variant<PointerTypes...> & v) noexcept
	{
		constexpr auto index = boost::mp11::mp_find<pointer_variant<PointerTypes...>, T>::value;
		return index == v.index();
	}

	template <class T, class ... PointerTypes>
	constexpr inline T get(const pointer_variant<PointerTypes...> & v)
	{
		constexpr auto idx = boost::mp11::mp_find<pointer_variant<PointerTypes...>, T>::value;
		if (v.index() != idx) throw;

		return static_cast<T>(v.pointer());
	}

	/************************************************************************/
	/*            pointer_variant visit implementation                      */
	/************************************************************************/
	namespace pointer_variant_detail
	{
		template <class... Contants>
		struct join_index_constants;

		template <std::size_t ... Index>
		struct join_index_constants<std::integral_constant<std::size_t, Index>...>
		{
			using type = std::index_sequence<Index...>;
		};

		template <class ... Constatns>
		using join_index_constants_t = typename join_index_constants<Constatns...>::type;

		template <class Type>
		using mp_from_sequence_variants = boost::mp11::mp_from_sequence<std::make_index_sequence<pointer_variant_size_v<ext::remove_cvref_t<Type>>>>;


		template <std::size_t ... Indexes, class Visitor, class ... Variants>
		inline auto dispatch_function1(std::index_sequence<Indexes...>, Visitor && vis, const Variants & ... args)
			-> std::invoke_result_t<Visitor, boost::mp11::mp_first<ext::remove_cvref_t<Variants>>...>
		{
			static_assert(sizeof...(Indexes) == sizeof...(Variants));

			return std::forward<Visitor>(vis)(
				reinterpret_cast<pointer_variant_alternative_t<Indexes, ext::remove_cvref_t<Variants>>>(args.pointer())...
			);
		}

		template <class Indexes, class Visitor, class ... Variants>
		inline auto dispatch_function2(Visitor && vis, const Variants & ... args)
			-> std::invoke_result_t<Visitor, boost::mp11::mp_first<ext::remove_cvref_t<Variants>>...>
		{
			return dispatch_function1(Indexes {}, std::forward<Visitor>(vis), args...);
		}


		template <class Visitor, class Variants, class Indexes>
		struct dispatch_table; // undefined

		template <class Visitor, class ... Variants, class ... Indexes>
		struct dispatch_table<Visitor, boost::mp11::mp_list<Variants...>, boost::mp11::mp_list<Indexes...>>
		{
			using result_type = std::invoke_result_t<Visitor, boost::mp11::mp_first<ext::remove_cvref_t<Variants>>...>;
			using sig_type = result_type(*)(Visitor &&, const Variants & ...);

			static constexpr sig_type dispatch_array[] = {&dispatch_function2<Indexes, Visitor, Variants...>...};
		};

		template <class ... Variants>
		constexpr auto dispatch_scale_v = (pointer_variant_size_v<Variants> * ... * 1);
		
		constexpr std::size_t dispatch_index()
		{
			return 0;
		}

		template <class Variant, class ... Rest>
		constexpr std::size_t dispatch_index(const Variant & var, const Rest & ... rest)
		{
			return var.index() * dispatch_scale_v<Rest...> + dispatch_index(rest...);
		}
	} // namespace pointer_variant_detail


	template <class Visitor, class ... Variants>
	constexpr auto visit(Visitor && vis, Variants && ... vars) ->
		std::enable_if_t<
			std::conjunction_v<is_pointer_variant<ext::remove_cvref_t<Variants>>...>,
			std::invoke_result_t<Visitor, boost::mp11::mp_first<ext::remove_cvref_t<Variants>>...>
		>
	{
		using namespace pointer_variant_detail;
		using namespace boost::mp11;
		
		using list_of_sequence_lists = mp_product<
			join_index_constants_t,
			mp_from_sequence_variants<Variants>...
			//boost::mp11::mp_from_sequence<std::make_index_sequence<pointer_variant_size_v<ext::remove_cvref_t<Variants>>>>...
		>;

		const auto dispatch_idx = dispatch_index(vars...);
		constexpr auto & dispatch_array = dispatch_table<Visitor, mp_list<ext::remove_cvref_t<Variants>...>, list_of_sequence_lists>::dispatch_array;
		return dispatch_array[dispatch_idx](std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
	}

	/************************************************************************/
	/*          pointer_variant relation operators                          */
	/************************************************************************/
	template <class ... PointerTypes>
	constexpr inline bool operator ==(const pointer_variant<PointerTypes...> & v1, const pointer_variant<PointerTypes...> & v2)
	{
		return v1.index() == v2.index() and v1.pointer() == v2.pointer();
	}

	template <class ... PointerTypes>
	constexpr inline bool operator !=(const pointer_variant<PointerTypes...> & v1, const pointer_variant<PointerTypes...> & v2)
	{
		return not operator ==(v1, v2);
	}

	template <class ... PointerTypes>
	constexpr inline bool operator <(const pointer_variant<PointerTypes...> & v1, const pointer_variant<PointerTypes...> & v2)
	{
		return v1.index() < v1.index() or v1.index() == v2.index() and v1.pointer() < v2.pointer();
	}

	template <class ... PointerTypes>
	constexpr inline bool operator >(const pointer_variant<PointerTypes...> & v1, const pointer_variant<PointerTypes...> & v2)
	{
		return operator <(v2, v1);
	}

	template <class ... PointerTypes>
	constexpr inline bool operator <=(const pointer_variant<PointerTypes...> & v1, const pointer_variant<PointerTypes...> & v2)
	{
		return not operator >(v1, v2);
	}

	template <class ... PointerTypes>
	constexpr inline bool operator >=(const pointer_variant<PointerTypes...> & v1, const pointer_variant<PointerTypes...> & v2)
	{
		return not operator <(v1, v2);
	}
}
