#include <QtCore/QStringBuilder>
#include <QtGui/QClipboard>

#include <QtWidgets/QShortcut>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolButton>

#include <QtWidgets/QMdiArea>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QApplication>

#include <ext/join_into.hpp>
#include <QtTools/ToolsBase.hpp>

#include <qtor/NotificationSystem.hqt>
#include <qtor/NotificationSystemExt.hqt>
#include <qtor/NotificationView.hqt>
#include <qtor/NotificationViewExt.hqt>
#include <qtor/NotificationViewDelegate.hqt>

#include <QtWidgets/QApplication>

namespace QtTools::NotificationSystem
{
	void NotificationView::OnFilteringChanged()
	{
		if (m_model)
		{
			decltype(m_filterString) fstr;
			decltype(m_filteredLevels) flvl;

			if (m_filterModes.testFlag(FilterByText))
				fstr = m_filterString;

			if (m_filterModes.testFlag(FilterByLevel))
				flvl = m_filteredLevels;

			if (flvl.none()) flvl.flip();

			m_model->SetFiltering(fstr, flvl);
			m_listView->viewport()->update();
		}
	}

	void NotificationView::NotificationLevelToggled()
	{
		NotificationLevelBitset val;
		val.set(Error, m_showErrors->isChecked());
		val.set(Warn,  m_showWarnings->isChecked());
		val.set(Info,  m_showInfos->isChecked());
		
		SetNotificationLevelFilter(val);
	}

	void NotificationView::SetupFiltering()
	{
		if (m_filterModes.testFlag(FilterByLevel))
		{
			m_showErrors->setVisible(true);
			m_showWarnings->setVisible(true);
			m_showInfos->setVisible(true);
			m_levelSeparator->setVisible(m_filterModes.testFlag(FilterByText));
		}
		else
		{
			m_showErrors->setVisible(false);
			m_showWarnings->setVisible(false);
			m_showInfos->setVisible(false);
			m_levelSeparator->setVisible(false);
		}

		if (m_filterModes.testFlag(FilterByText))
			m_textFilter->show();
		else
			m_textFilter->hide();
	}

	void NotificationView::SetFilterMode(FilterModeFlags modes)
	{
		m_filterModes = modes;

		SetupFiltering();
		OnFilteringChanged();
		Q_EMIT FilterModeChanged(m_filterModes);
	}

	void NotificationView::SetFilter(QString newFilter)
	{
		m_filterString = std::move(newFilter);
		OnFilteringChanged();
		Q_EMIT FilterChanged(m_filterString);
	}

	void NotificationView::SetNotificationLevelFilter(NotificationLevelBitset filtered)
	{
		m_filteredLevels = filtered;
		OnFilteringChanged();
		Q_EMIT NotificationLevelFilterChanged(m_filteredLevels);
	}

	QString NotificationView::ClipboardText(const Notification & n) const
	{
		auto title = n.Title();
		auto text = n.PlainText();
		auto timestamp = locale().toString(n.Timestamp(), QLocale::ShortFormat);

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

		auto indexes = m_listView->selectionModel()->selectedRows();
		auto plainTexts = indexes | transformed([this](auto & idx) { return ClipboardText(m_model->GetItem(idx.row())); });
		auto richTexts  = indexes | transformed([this](auto & idx) { return m_model->GetItem(idx.row()).Text(); });

		ext::join_into(plainTexts, plainSep, plainText);
		ext::join_into(richTexts, richSep, richText);

		QMimeData * mime = new QMimeData;
		mime->setText(plainText);
		mime->setHtml(richText);

		qApp->clipboard()->setMimeData(mime);
	}

	//QMenu * NotificationView::CreateItemMenu(const QModelIndex & idx)
	//{
	//	if (not idx.isValid()) return nullptr;

	//	auto * menu = new QMenu(this);
	//	connect(menu, &QMenu::aboutToHide, menu, &QObject::deleteLater); // auto delete on hide

	//	menu->addActions(actions());
	//	return menu;
	//}

	//void NotificationView::contextMenuEvent(QContextMenuEvent * ev)
	//{
	//	ev->accept();
	//	auto pos = ev->globalPos();

	//	auto * viewport = m_listView->viewport();
	//	auto viewPos = viewport->mapFromGlobal(pos);

	//	// process only menu from QListView
	//	if (not viewport->contentsRect().contains(viewPos))
	//		return;

	//	auto idx = m_listView->indexAt(viewPos);
	//	auto * menu = CreateItemMenu(idx);
	//	if (menu) menu->popup(pos);
	//}

	/************************************************************************/
	/*                    Init Methods                                      */
	/************************************************************************/
	void NotificationView::ConnectModel()
	{
		auto * model = m_model.get();
		m_listView->setModel(model);
		m_textFilter->clear();

		//connect(model, &QAbstractItemModel::layoutChanged, this, &NotificationView::ModelChanged);
		//connect(model, &QAbstractItemModel::modelReset, this, &NotificationView::ModelChanged);
		//connect(model, &AbstractNotificationModel::SortingChanged, this, &NotificationView::OnSortingChanged);

		//if (m_sortColumn >= 0) m_model->sort(m_sortColumn, m_sortOrder);
		//m_sortMenu = CreateSortMenu();
	}

	void NotificationView::DisconnectModel()
	{
		m_listView->setModel(nullptr);
		//m_sizeHint = m_defMinSizeHint;
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
		auto model = std::make_shared<NotificationModel>(center.GetStore());
		SetModel(std::move(model));
	}

	NotificationView::NotificationView(QWidget * parent /* = nullptr */) : QFrame(parent)
	{
		setupUi();
		connectSignals();
		setupActions();
		retranslateUi();

		SetupFiltering();
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
				m_textFilter, static_cast<void (QLineEdit::*)()>(&QLineEdit::setFocus));

		connect(m_textFilter, &QLineEdit::textChanged, this, &NotificationView::SetFilter);

		connect(m_showErrors,   &QAction::toggled, this, &NotificationView::NotificationLevelToggled);
		connect(m_showWarnings, &QAction::toggled, this, &NotificationView::NotificationLevelToggled);
		connect(m_showInfos,    &QAction::toggled, this, &NotificationView::NotificationLevelToggled);
	}

	void NotificationView::setupUi()
	{
		m_verticalLayout = new QVBoxLayout(this);

		m_listView = new QListView(this);
		m_listView->setAlternatingRowColors(true);
		m_listView->setTabKeyNavigation(false);
		m_listView->setModelColumn(1);
		//m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
		m_listView->setWordWrap(true);
		m_listView->setMouseTracking(true);

		//m_listView->setResizeMode(QListView::Adjust);
		//m_listView->setLayoutMode(QListView::SinglePass);
		
		m_listDelegate = new NotificationViewDelegate(this);
		m_listView->setItemDelegate(m_listDelegate);

		m_listView->setObjectName("listView");
		m_listDelegate->setObjectName("listDelegate");

		setupToolbar();

		m_verticalLayout->addWidget(m_toolBar);
		m_verticalLayout->addWidget(m_listView);
	}

	void NotificationView::setupToolbar()
	{
		m_toolBar = new QToolBar(this);
		m_textFilter = new QLineEdit(this);
		m_textFilter->setClearButtonEnabled(true);

		m_toolBar->setObjectName("toolBar");
		m_textFilter->setObjectName("textFilter");

		m_toolBar->setIconSize(QtTools::ToolBarIconSizeForLineEdit(m_textFilter));
		m_toolBar->layout()->setContentsMargins(0, 0, 0, 0);
		m_toolBar->layout()->setSpacing(2);


		{
			using QtTools::LoadIcon;
			QIcon infoIcon, warnIcon, errorIcon;

			// see https://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
			errorIcon = LoadIcon("dialog-error", QStyle::SP_MessageBoxCritical);
			warnIcon  = LoadIcon("dialog-warning", QStyle::SP_MessageBoxWarning);
			infoIcon  = LoadIcon("dialog-information", QStyle::SP_MessageBoxInformation);

			m_showErrors = m_toolBar->addAction(errorIcon, tr("&Error"));
			m_showErrors->setToolTip(tr("Show error notifications(Alt+E)"));
			m_showErrors->setShortcut(QKeySequence(tr("Alt+E")));
			m_showErrors->setCheckable(true);
			m_showErrors->setObjectName("showErrors");

			m_showWarnings = m_toolBar->addAction(warnIcon, tr("&Warning"));
			m_showWarnings->setToolTip(tr("Show warning notifications(Alt+W)"));
			m_showWarnings->setShortcut(QKeySequence(tr("Alt+W")));
			m_showWarnings->setCheckable(true);
			m_showWarnings->setObjectName("showWarnings");

			m_showInfos = m_toolBar->addAction(infoIcon, tr("&Info"));
			m_showInfos->setToolTip(tr("Show info notifications(Alt+I)"));
			m_showInfos->setShortcut(QKeySequence(tr("Alt+I")));
			m_showInfos->setCheckable(true);
			m_showInfos->setObjectName("showInfos");

			m_levelSeparator = m_toolBar->addSeparator();
			m_levelSeparator->setObjectName("levelSeparator");
		}

		m_toolBar->addWidget(m_textFilter);
	}

	void NotificationView::setupActions()
	{
		for (auto * action : actions()) removeAction(action);

		auto * copyAction = new QAction(tr("&Copy"), this);
		copyAction->setShortcut(QKeySequence::Copy);
		copyAction->setObjectName("copyAction");
		connect(copyAction, &QAction::triggered, this, &NotificationView::CopySelectedIntoClipboard);

		addAction(copyAction);
		setContextMenuPolicy(Qt::ActionsContextMenu);
	}

	void NotificationView::retranslateUi()
	{
		//: filter shortcut in generic BasicTableWidget, shown as placeholder in EditWidget
		m_textFilter->setPlaceholderText(tr("Text filter(Ctrl+F)"));
	}
}
