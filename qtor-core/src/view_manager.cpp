#include <qtor/view_manager.hpp>

namespace qtor
{
	void view_manager::start_subscription()
	{
		if (not m_subsription_handle)
			subscribe();
		else
			m_subsription_handle.resume();
	}

	void view_manager::stop_subscription()
	{
		m_subsription_handle.pause();
	}

	unsigned view_manager::view_addref()
	{
		if (m_viewcount++ == 0)
			start_subscription();

		return m_viewcount;
	}

	unsigned view_manager::view_release()
	{
		assert(m_viewcount);
		if (--m_viewcount == 0)
			stop_subscription();

		return m_viewcount;
	}
}
