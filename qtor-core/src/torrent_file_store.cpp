#include <qtor/torrent_file_store.hpp>
#include <QtCore/QPointer>
#include <QtCore/QDebug>

namespace qtor
{
	void torrent_file_store::refresh()
	{
		auto * executor = m_source->get_gui_executor();
		assert(executor);

		auto ffiles = m_source->get_torrent_files(m_torrent_id);
		executor->submit(std::move(ffiles), [this](auto ffiles) { assign_records(std::move(ffiles.get())); });
	}
}
