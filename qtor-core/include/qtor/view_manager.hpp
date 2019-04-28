#pragma once
#include <ext/net/subscription_handle.hpp>

namespace qtor
{
	class view_manager
	{
	protected:
		unsigned m_viewcount = 0;
		ext::net::subscription_handle m_subsription_handle;

	protected:
		virtual auto subscribe() -> ext::net::subscription_handle = 0;
		virtual void start_subscription();
		virtual void stop_subscription();

	public:
		/// current number of connected views
		unsigned view_count() { return m_viewcount; }
		/// adds view, resumes subscription if needed
		unsigned view_addref();
		/// release view, decrements view counter. If there are no more connected views - pauses subscription
		unsigned view_release();
	};


	/// similar to std::shared_ptr, but has unique_ptr semantics,
	/// and calls view_addref, view_release on acquire/release operations
	template <class Store>
	class view_manager_ref
	{
	public:
		static_assert(std::is_base_of_v<view_manager, Store>);

		using store_type = Store;
		using pointer_type = std::shared_ptr<store_type>;
		using element_type = typename pointer_type::element_type;

	private:
		pointer_type m_ptr;

	public:
		void reset() { reset(nullptr); }
		void reset(pointer_type ref);
		pointer_type release() { return std::move(m_ptr); }

		auto get() const noexcept { return m_ptr.get(); }
		const auto & get_smart_ptr() const noexcept { return  m_ptr; }

		explicit operator bool() const noexcept { return static_cast<bool>(m_ptr); }
		operator pointer_type() const noexcept { return m_ptr; }

		auto * operator ->() const noexcept { return m_ptr.get(); }
		decltype(auto) operator *() const noexcept { return *m_ptr; }

	public:
		void swap(view_manager_ref & other) { m_ptr.swap(other.m_ptr); }

	public:
		view_manager_ref() = default;
		~view_manager_ref();

		view_manager_ref(pointer_type ptr);
		view_manager_ref & operator =(pointer_type ptr);

		view_manager_ref(const view_manager_ref &) = delete;
		view_manager_ref & operator =(const view_manager_ref &) = delete;
	};

	template <class Store>
	inline view_manager_ref<Store>::view_manager_ref(pointer_type ptr)
	    : m_ptr(std::move(ptr))
	{
		if (m_ptr)
			m_ptr->view_addref();
	}

	template <class Store>
	inline auto view_manager_ref<Store>::operator=(pointer_type ptr) -> view_manager_ref &
	{
		reset(std::move(ptr));
		return *this;
	}

	template <class Store>
	inline view_manager_ref<Store>::~view_manager_ref()
	{
		if (m_ptr)
			m_ptr->view_release();
	}

	template <class Store>
	void view_manager_ref<Store>::reset(pointer_type ref)
	{
		if (m_ptr == ref) return;

		if (m_ptr)
			m_ptr->view_release();

		m_ptr = std::move(ref);
		if (m_ptr)
			m_ptr->view_addref();
	}

	template <class Store>
	inline void swap(view_manager_ref<Store> & v1, view_manager_ref<Store> & v2)
	{
		v1.swap(v2);
	}
}
