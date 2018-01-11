#include <QtTools/ToolsBase.hpp>
#include <qtor/NotificationSystem.hqt>

namespace QtTools
{
	viewed::refilter_type NotificationSystem::NotificationFilter::set_expr(QString search)
	{
		return viewed::refilter_type::same;
	}

	bool NotificationSystem::NotificationFilter::matches(const Notification & n) const noexcept
	{
		return true;
	}

	bool NotificationSystem::NotificationFilter::always_matches() const noexcept
	{
		return true;
	}

	NotificationSystem::Model::Model(std::shared_ptr<Store> store, QObject * parent /* = nullptr */)
		: view_type(store.get()), base_type(parent)
	{
		assert(store);

		m_owner_store = std::move(store);

		// from view_base_type
		connect_signals();
		reinit_view();
	}

	auto NotificationSystem::Model::GetItem(int row) const -> const Notification *
	{
		return m_store.at(row);
	}

	void NotificationSystem::Model::FilterBy(QString expr)
	{
		auto rtype = m_filter_pred.set_expr(expr);
		refilter_and_notify(rtype);
	}

	int NotificationSystem::Model::FullRowCount() const
	{
		return qint(m_owner_store->size());
	}

	int NotificationSystem::Model::rowCount(const QModelIndex & parent /*= QModelIndex()*/) const
	{
		return qint(m_store.size());
	}

	QVariant NotificationSystem::Model::data(const QModelIndex & index, int role /*= Qt::DisplayRole*/) const
	{
		if (!index.isValid())
			return {};

		int row = index.row();
		int column = index.column();

		switch (role)
		{
			//case Qt::UserRole:
			//	return QVariant::fromValue(GetItem(row));

			case Qt::DisplayRole:
			case Qt::ToolTipRole:
				return m_store[row]->Text();

			default:          return {};
		}
	}
}
