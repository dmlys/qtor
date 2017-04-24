#pragma once
#include <ext/netlib/subscription_handle.hpp>

namespace qtor
{
	class view_manager
	{
	protected:
		unsigned m_viewcount = 0;
		ext::netlib::subscription_handle m_subsription_handle;

	protected:
		virtual auto subscribe() -> ext::netlib::subscription_handle = 0;
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
}
