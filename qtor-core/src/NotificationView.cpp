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

		setupToolbar();

		m_verticalLayout->addWidget(m_toolBar);
		m_verticalLayout->addWidget(m_listView);
	}

	static QIcon loadIcon(const QString & themeIcon, QStyle::StandardPixmap fallback)
	{
		if (QIcon::hasThemeIcon(themeIcon))
			return QIcon::fromTheme(themeIcon);

		return qApp->style()->standardIcon(fallback);
	}

	static QSize ToolBarIconSizeForLineEdit(QLineEdit * lineEdit)
	{
#ifdef Q_OS_WIN
		// on windows pixelMetric(QStyle::PM_DefaultFrameWidth) returns 1,
		// but for QLineEdit internal code actually uses 2
		constexpr auto frameWidth = 2;
#else
		const auto frameWidth = lineEdit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
#endif

		lineEdit->adjustSize();
		auto height = lineEdit->size().height();
		height -= frameWidth;

		return {height, height};
	}

	void NotificationView::setupToolbar()
	{
		m_toolBar = new QToolBar(this);
		m_rowFilter = new QLineEdit(this);
		m_rowFilter->setClearButtonEnabled(true);

		m_toolBar->setIconSize(ToolBarIconSizeForLineEdit(m_rowFilter));
		m_toolBar->layout()->setContentsMargins(0, 0, 0, 0);
		m_toolBar->layout()->setSpacing(2);


		{
			QIcon infoIcon, warnIcon, errorIcon;

			// see https://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
			errorIcon = loadIcon("dialog-error", QStyle::SP_MessageBoxCritical);
			warnIcon  = loadIcon("dialog-warning", QStyle::SP_MessageBoxWarning);
			infoIcon  = loadIcon("dialog-information", QStyle::SP_MessageBoxInformation);

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

		m_toolBar->addWidget(m_rowFilter);
	}

	void NotificationView::setupActions()
	{
		for (auto * action : actions()) removeAction(action);

		auto * copyAction = new QAction(tr("&Copy"), this);
		copyAction->setShortcut(QKeySequence::Copy);
		connect(copyAction, &QAction::triggered, this, &NotificationView::CopySelectedIntoClipboard);

		addAction(copyAction);
		setContextMenuPolicy(Qt::ActionsContextMenu);
	}

	void NotificationView::retranslateUi()
	{
		//: filter shortcut in generic BasicTableWidget, shown as placeholder in EditWidget
		m_rowFilter->setPlaceholderText(tr("Row filter(Ctrl+F)"));
	}
}
