#include <ext/join_into.hpp>

#include <QtGui/QClipboard>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QMenu>

#include <QtTools/ToolsBase.hpp>
#include <qtor/NotificationSystem.hqt>
#include <qtor/NotificationSystemExt.hqt>


namespace QtTools::NotificationSystem
{
	void NotificationView::OnFilterChanged()
	{
		SetFilter(m_rowFilter->text());
	}

	void NotificationView::SetFilter(QString newFilter)
	{
		m_filterString = std::move(newFilter);
		if (m_model)
		{
			m_model->SetFilter(m_filterString);
			m_listView->viewport()->update();
		}
	}

	//bool NotificationView::eventFilter(QObject * watched, QEvent * event)
	//{
	//	if (event->type() == QEvent::KeyPress)
	//	{
	//		auto * keyEvent = static_cast<QKeyEvent *>(event);
	//		if (keyEvent->matches(QKeySequence::Copy) and m_listView == watched)
	//		{
	//			auto indexes = m_listView->selectionModel()->selectedIndexes();
	//			auto texts = indexes | boost::adaptors::transformed([this](auto & idx) { return m_listDelegate->GetText(idx); });

	//			QString text;
	//			auto sep = "\n" + QString(80, '-') + "\n";
	//			ext::join_into(texts, sep, text);

	//			qApp->clipboard()->setText(text);
	//			return true;
	//		}
	//	}

	//	return false;
	//}

	/************************************************************************/
	/*                    Init Methods                                      */
	/************************************************************************/
	void NotificationView::ConnectModel()
	{
		auto * model = m_model.get();
		m_listView->setModel(model);
		m_rowFilter->clear();

		//connect(model, &QAbstractItemModel::layoutChanged, this, &NotificationView::ModelChanged);
		//connect(model, &QAbstractItemModel::modelReset, this, &NotificationView::ModelChanged);
		//connect(model, &AbstractNotificationModel::SortingChanged, this, &NotificationView::OnSortingChanged);

		//if (m_sortColumn >= 0) m_model->sort(m_sortColumn, m_sortOrder);
		//m_sortMenu = CreateSortMenu();
	}

	void NotificationView::DisconnectModel()
	{
		m_listView->setModel(nullptr);
		m_sizeHint = m_defMinSizeHint;
	}

	void NotificationView::SetModel(std::shared_ptr<AbstractNotificationModel> model)
	{
		// both null or valid
		if (m_model) m_model->disconnect(this);
		m_model = std::move(model);

		if (m_model)
		{
			ConnectModel();
			updateGeometry();
		}
		else
		{
			DisconnectModel();
		}
	}

	NotificationView::NotificationView(QWidget * parent /* = nullptr */) : QFrame(parent)
	{
		setupUi();
		connectSignals();
		retranslateUi();
	}

	void NotificationView::connectSignals()
	{
		//: filter shortcut for a QListView, probably should not be translated
		auto * filterShortcut = new QShortcut(QKeySequence(tr("Ctrl+F")), this);
		connect(filterShortcut, &QShortcut::activated,
				m_rowFilter, static_cast<void (QLineEdit::*)()>(&QLineEdit::setFocus));

		connect(m_rowFilter, &QLineEdit::textChanged, this, &NotificationView::OnFilterChanged);
	}

	void NotificationView::setupUi()
	{
		m_verticalLayout = new QVBoxLayout(this);
		m_rowFilter = new QLineEdit(this);
		m_rowFilter->setClearButtonEnabled(true);

		m_listView = new QListView(this);
		m_listView->setAlternatingRowColors(true);
		m_listView->setTabKeyNavigation(false);
		m_listView->setModelColumn(1);
		m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
		m_listView->setMouseTracking(true);

		m_listDelegate = new SimpleNotificationDelegate(this);
		m_listView->setItemDelegate(m_listDelegate);
		//m_listView->installEventFilter(this);

		m_verticalLayout->addWidget(m_rowFilter);
		m_verticalLayout->addWidget(m_listView);
	}

	void NotificationView::retranslateUi()
	{
		//: filter shortcut in generic BasicTableWidget, shown as placeholder in EditWidget
		m_rowFilter->setPlaceholderText(tr("Row filter(Ctrl+F)"));
	}
}
