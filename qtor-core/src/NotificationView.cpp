#include <QtCore/QStringBuilder>
#include <QtGui/QClipboard>

#include <QtWidgets/QShortcut>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QMenu>

#include <ext/join_into.hpp>
#include <QtTools/ToolsBase.hpp>

#include <qtor/NotificationSystem.hqt>
#include <qtor/NotificationSystemExt.hqt>
#include <qtor/NotificationView.hqt>
#include <qtor/NotificationViewExt.hqt>
#include <qtor/NotificationViewDelegate.hqt>


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

	QString NotificationView::ClipboardText(const Notification & n) const
	{
		auto title = n.Title();
		auto text = n.PlainText();
		auto timestamp = n.Timestamp().toString(Qt::DateFormat::DefaultLocaleShortDate);

		return title % "  " % timestamp
		     % QStringLiteral("\n")
		     % text;
	}

	void NotificationView::CopySelectedIntoClipboard()
	{
		using boost::adaptors::transformed;
		//const QChar newPar = QChar::ParagraphSeparator;
		const auto plainSep = "\n" + QString(80, '-') + "\n";
		const auto richSep = QStringLiteral("<hr>");

		QString plainText, richText;

		auto indexes = m_listView->selectionModel()->selectedIndexes();
		auto plainTexts = indexes | transformed([this](auto & idx) { return ClipboardText(m_model->GetItem(idx.row())); });
		auto richTexts  = indexes | transformed([this](auto & idx) { return m_model->GetItem(idx.row()).Text(); });

		ext::join_into(plainTexts, plainSep, plainText);
		ext::join_into(richTexts, richSep, richText);

		QMimeData * mime = new QMimeData;
		mime->setText(plainText);
		mime->setHtml(richText);

		qApp->clipboard()->setMimeData(mime);
	}

	QMenu * NotificationView::CreateItemMenu(const QModelIndex & idx)
	{
		if (not idx.isValid()) return nullptr;

		QAction * action;
		auto * menu = new QMenu(this);
		connect(menu, &QMenu::aboutToHide, menu, &QObject::deleteLater); // auto delete on hide

		action = new QAction(tr("&Copy"), menu);
		connect(action, &QAction::triggered, this, &NotificationView::CopySelectedIntoClipboard);
		menu->addAction(action);

		return menu;
	}

	void NotificationView::contextMenuEvent(QContextMenuEvent * ev)
	{
		ev->accept();
		auto pos = ev->globalPos();

		auto * viewport = m_listView->viewport();
		auto viewPos = viewport->mapFromGlobal(pos);

		// process only menu from QListView
		if (not viewport->contentsRect().contains(viewPos))
			return;

		auto idx = m_listView->indexAt(viewPos);
		auto * menu = CreateItemMenu(idx);
		if (menu) menu->popup(pos);
	}

	bool NotificationView::eventFilter(QObject * watched, QEvent * event)
	{
		if (event->type() == QEvent::KeyPress)
		{
			auto * keyEvent = static_cast<QKeyEvent *>(event);
			if (keyEvent->matches(QKeySequence::Copy) and m_listView == watched)
			{
				CopySelectedIntoClipboard();
				return true;
			}
		}

		return false;
	}

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

	void NotificationView::Init(NotificationCenter & center)
	{
		SetModel(center.CreateModel());
	}

	NotificationView::NotificationView(QWidget * parent /* = nullptr */) : QFrame(parent)
	{
		setupUi();
		connectSignals();
		retranslateUi();
	}

	NotificationView::NotificationView(NotificationCenter & center, QWidget * parent /* = nullptr */)
		: NotificationView(parent)
	{
		Init(center);
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
		m_listView->setWordWrap(true);
		m_listView->setMouseTracking(true);

		m_listDelegate = new NotificationViewDelegate(this);
		m_listView->setItemDelegate(m_listDelegate);
		m_listView->installEventFilter(this);

		m_verticalLayout->addWidget(m_rowFilter);
		m_verticalLayout->addWidget(m_listView);
	}

	void NotificationView::retranslateUi()
	{
		//: filter shortcut in generic BasicTableWidget, shown as placeholder in EditWidget
		m_rowFilter->setPlaceholderText(tr("Row filter(Ctrl+F)"));
	}
}
