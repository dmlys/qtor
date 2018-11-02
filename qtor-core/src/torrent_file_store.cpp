#include <qtor/torrent_file_store.hpp>
#include <QtCore/QPointer>
#include <QtCore/QDebug>

namespace qtor
{
	template <class Future, class Handler>
	auto then_in_gui(QtTools::GuiQueue * queue, Future f, Handler handler) -> std::enable_if_t<ext::is_future_type_v<Future>, ext::future<void>>
	{
		assert(queue);
		if (f.is_ready())
		{
			handler(f.get());
			return ext::make_ready_future();
		}

		auto continuation = [queue_ptr = QPointer<QtTools::GuiQueue>(queue), handler = std::move(handler)]
			(auto f) mutable
		{
			auto * queue = queue_ptr.data();
			if (not queue)
			{
				// if this happened at application shutdown - it nay be not a problem
				qWarning() << "then_in_gui::continuation fired, but GuiQueue expired, continuation lost";
			}
			else
			{
				queue->Add(std::bind(std::move(handler), f.get()));
			}
		};

		return f.then(std::move(continuation));
	}

	template <class Future, class Handler>
	auto then_in_gui(QtTools::GuiQueue * queue, Future f, QObject * bound, Handler handler) -> std::enable_if_t<ext::is_future_type_v<Future>, ext::future<void>>
	{
		assert(queue);
		assert(bound);

		if (f.is_ready())
		{
			handler(f.get());
			return ext::make_ready_future();
		}

		auto continuation = [queue_ptr = QPointer(queue), handler = std::move(handler), bound_ptr = QPointer(bound)]
			(auto f) mutable
		{
			auto * queue = queue_ptr.data();
			if (not queue)
			{
				// if this happened at application shutdown - it nay be not a problem
				qWarning() << "then_in_gui::continuation fired, but GuiQueue expired, continuation lost";
			}
			else
			{
				auto proc = [handler = std::move(handler), bound_ptr = std::move(bound_ptr)](auto val) mutable
				{
					auto * bound = bound_ptr.data();
					if (bound) handler(std::move(val));
				};

				queue->Add(std::move(proc));
			}
		};

		return f.then(std::move(continuation));
	}


	void torrent_file_store::refresh()
	{
		auto gqueue = m_source->get_gui_queue();
		assert(gqueue);

		auto ffiles = m_source->get_torrent_files(m_torrent_id);
		then_in_gui(gqueue, std::move(ffiles), [this](auto files) { assign_records(std::move(files)); });
	}
}
