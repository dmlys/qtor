#include <qtor/TorrentsView.hqt>
#include <qtor/MainWindow.hqt>

#include <QtGui/QClipboard>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QMenu>

#include <QtTools/ItemViewUtils.hpp>
#include <QtTools/HeaderConfigurationWidget.hqt>

#include <QtTools/ToolsBase.hpp>
#include <ext/join_into.hpp>

namespace qtor
{
	void TorrentsView::OnFilterChanged()
	{
		SetFilter(m_rowFilter->text());
	}

	void TorrentsView::SetFilter(QString newFilter)
	{
		m_filterString = std::move(newFilter);
		m_nameDelegate->SetFilterText(m_filterString);
		m_listDelegate->SetFilterText(m_filterString);

		if (m_model)
		{
			m_model->SetFilter(m_filterString);
			m_itemView->viewport()->update();
		}
	}

	void TorrentsView::Sort(int column, Qt::SortOrder order)
	{
		if (not m_model) return;

		// model will emit signal and we will adjust ourselves in OnSortingChanged
		m_model->sort(m_sortColumn, m_sortOrder);
	}

	void TorrentsView::OnSortingChanged(int column, Qt::SortOrder order)
	{
		m_sortColumn = column;
		m_sortOrder = order;

		if (m_sortMenu or column >= 0)
		{
			auto actions = m_sortMenu->actions();
			actions[column + 3]->setChecked(true);
			actions[order]->setChecked(true);
		}

		Q_EMIT SortingChanged(m_sortColumn, m_sortOrder);
	}

	/************************************************************************/
	/*                    Sort submenu                                      */
	/************************************************************************/
	QMenu * TorrentsView::CreateSortMenu()
	{
		if (not m_model) return nullptr;

		auto changeOrder = [this](auto checked)
		{
			if (not m_model or not checked or m_sortColumn < 0) return;
			
			auto * action = static_cast<QAction *>(QObject::sender());
			Qt::SortOrder newOrder = qvariant_cast<Qt::SortOrder>(action->data());
			m_model->sort(m_sortColumn, newOrder);
		};

		auto changeColumn = [this](auto checked)
		{
			if (not m_model or not checked) return;

			auto * action = static_cast<QAction *>(QObject::sender());
			int newColumn = qvariant_cast<int>(action->data());
			m_model->sort(newColumn, m_sortOrder);
		};

		auto * menu = new QMenu(tr("&Sort by"), this);
		QAction * action;
		QActionGroup * group;
		
		group = new QActionGroup(this);
		//: sort ascending
		action = menu->addAction(tr("&Ascending"));
		action->setCheckable(true);
		action->setData(Qt::AscendingOrder);
		group->addAction(action);
		connect(action, &QAction::triggered, this, changeOrder);

		//: sort ascending
		action = menu->addAction(tr("&Descending"));
		action->setData(Qt::DescendingOrder);
		action->setCheckable(true);
		group->addAction(action);
		connect(action, &QAction::triggered, this, changeOrder);

		group = new QActionGroup(this);
		action = menu->addSection(tr("Colunms"));
		group->addAction(action);

		for (int i = 0, n = m_model->columnCount(); i < n; ++i)
		{
			auto name = m_model->FieldName(i);
			action = menu->addAction(name);
			action->setData(i);
			action->setCheckable(true);
			group->addAction(action);
			connect(action, &QAction::triggered, this, changeColumn);
		}

		if (m_sortColumn >= 0)
		{
			auto actions = m_sortMenu->actions();
			actions[m_sortColumn + 3]->setChecked(true);
			actions[m_sortOrder]->setChecked(true);
		}
		return menu;
	}

	/************************************************************************/
	/*                Context Menu                                          */
	/************************************************************************/
	void TorrentsView::OpenHeaderConfigurationWidget()
	{
		if (not m_headerModel)
		{
			m_headerModel = new QtTools::HeaderControlModel(this);
			m_headerModel->Track(m_tableView->horizontalHeader());

			if (m_headerConfig)
				m_headerModel->Configurate(*m_headerConfig);
		}


		auto * confWgt = findChild<QtTools::HeaderConfigurationWidget *>("ConfigurationWidget");
		if (confWgt)
		{
			confWgt->show();
			confWgt->activateWindow();
			return;
		}
	
		confWgt = new QtTools::HeaderConfigurationWidget(*m_headerModel, this);
		confWgt->setObjectName("ConfigurationWidget");
		confWgt->show();
		confWgt->activateWindow();
	}
	
	void TorrentsView::TableSettings()
	{
		OpenHeaderConfigurationWidget();
	}
	
	void TorrentsView::ResizeColumnsToContents()
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
	
		// what we need is done by resizeColumnsToContents,
		// but it's also takes into account headers, and i want without them - have to do by hand
		m_tableView->resizeColumnsToContents();
	
		auto * header = m_tableView->horizontalHeader();
		auto minimum = header->minimumSectionSize();
		int count = header->count();
	
		for (int i = 0; i < count; ++i)
		{
			auto li = header->logicalIndex(i);
			if (not header->isSectionHidden(i))
			{
				// for some unknown reason virtual method sizeHintForColumn is declared as protected in QTableView,
				// through it's public QAbstractItemView. call through base class
				auto hint = static_cast<QAbstractItemView *>(m_tableView)->sizeHintForColumn(li);
				header->resizeSection(li, std::max(hint, minimum));
			}
		}
		
		QApplication::restoreOverrideCursor();
	}

	void TorrentsView::CopySelectedIntoClipboard()
	{
		auto indexes = m_listView->selectionModel()->selectedRows();
		auto texts = indexes | boost::adaptors::transformed([this](auto & idx) { return m_listDelegate->GetText(idx); });

		QString text;
		auto sep = "\n" + QString(80, '-') + "\n";
		ext::join_into(texts, sep, text);

		qApp->clipboard()->setText(text);
	}

	QMenu * TorrentsView::CreateItemMenu(const QModelIndex & idx)
	{
		QAction * action;
		auto * menu = new QMenu(this);
		connect(menu, &QMenu::aboutToHide, menu, &QObject::deleteLater); // auto delete on hide

		if (m_sortMenu)
		{
			menu->addMenu(m_sortMenu);
			menu->addSeparator();
		}

		action = new QAction(tr("Table &settings..."), menu);
		connect(action, &QAction::triggered, this, &TorrentsView::TableSettings);
		menu->addAction(action);
		
		//: TableView context menu item
		action = new QAction(tr("&Resize columns to content"), menu);
		connect(action, &QAction::triggered, this, &TorrentsView::ResizeColumnsToContents);
		menu->addAction(action);

		menu->addSeparator();
		action = new QAction(tr("&Table mode"), menu);
		connect(action, &QAction::triggered, this, [this] { SetViewMode(TableMode); });
		menu->addAction(action);

		action = new QAction(tr("&List mode"), menu);
		connect(action, &QAction::triggered, this, [this] { SetViewMode(ListMode); });
		menu->addAction(action);

/*		if (idx.isValid())
		{
			QPersistentModelIndex pidx = idx;

#define CONNECT(METHOD)                                                                     \
			action->setData(pidx);                                                          \
			connect(action, &QAction::triggered, this, [this]                               \
			{                                                                               \
				auto * action = static_cast<QAction *>(QObject::sender());                  \
				QModelIndex idx = qvariant_cast<QPersistentModelIndex>(action->data());     \
				m_parent->METHOD(idx);                                                      \
			})                                                                              \

			action = menu->addAction(tr("&Properties"));
			action->setIcon(QIcon::fromTheme("document-properties"));
			action->setShortcut(QKeySequence("Alt+Enter"));
			CONNECT(OpenTorrentLocationSettings);

			action = menu->addAction(tr("Open Fold&er"));
			action->setIcon(QIcon::fromTheme("folder-open"));
			action->setShortcut(QKeySequence("Alt+E"));
			CONNECT(OpenTorrentFolder);

			action = menu->addAction(tr("Start"));
			CONNECT(StartTorrent);

			action = menu->addAction(tr("start Now"));
			CONNECT(StartTorrentNow);

			action = menu->addAction(tr("Stop"));
			CONNECT(StopTorrent);

			action = menu->addAction(tr("Announce"));
			CONNECT(AnnounceTorrent);

			action = menu->addAction(tr("Remove"));
			CONNECT(DeleteTorrent);

			action = menu->addAction(tr("Remove and Delete data"));
			CONNECT(PurgeTorrent);



#undef CONNECT
		}
*/

		return menu;
	}

	void TorrentsView::contextMenuEvent(QContextMenuEvent * ev)
	{
		ev->accept();
		auto pos = ev->globalPos();

		auto * viewport = m_itemView->viewport();
		auto viewPos = viewport->mapFromGlobal(pos);

		// process only menu from QTableView
		if (not viewport->contentsRect().contains(viewPos))
			return;

		auto idx = m_tableView->indexAt(viewPos);
		auto * menu = CreateItemMenu(idx);
		if (menu) menu->popup(pos);
	}

	QSize TorrentsView::sizeHint() const
	{
		// maximum size - half screen
		QSize maxSize = QApplication::desktop()->screenGeometry().size();
		maxSize /= 2;
		// but no more than maximumSize()
		maxSize = maxSize.boundedTo(maximumSize());

		// if we are in QMdiArea, then our sizeHint should not be more than one third of QMdiArea size
		if (auto mdi = QtTools::FindAncestor<QMdiArea>(this))
		{
			maxSize = mdi->size();
			maxSize /= 3;
		}

		// additional size - size of all layout'а
		// minus size of tableView, size of which we calculate ourself.
		// QTableView::sizeHint in fact always reutrn dummy size: 256:192
		QSize addSz = QWidget::sizeHint() - m_tableView->sizeHint();
		maxSize -= addSz;

		auto sz = QtTools::ItemViewSizeHint(m_tableView, m_sizeHint, maxSize);
		return sz += addSz;
	}

	bool TorrentsView::eventFilter(QObject * watched, QEvent * event)
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
	void TorrentsView::SetViewMode(ViewMode mode)
	{
		switch (mode)
		{
			case TableMode:
				m_viewMode = mode;

				m_listView->hide();
				m_tableView->show();
				m_itemView = m_tableView;
				break;

			case ListMode:
				m_viewMode = mode;

				m_tableView->hide();
				m_listView->show();
				m_itemView = m_listView;
				break;

			default:
				break;
		}

		setFocusProxy(m_itemView);
		QWidget::setTabOrder(m_rowFilter, m_itemView);
	}

	void TorrentsView::ConnectModel()
	{
		auto * model = m_model.get();
		m_tableView->setModel(model);
		m_listView->setModel(model);
		m_rowFilter->clear();
		
		int nameCol = model->MetaToViewIndex(torrent::Name);
		m_tableView->setItemDelegateForColumn(nameCol, m_nameDelegate);

		//connect(model, &QAbstractItemModel::layoutChanged, this, &TorrentsView::ModelChanged);
		//connect(model, &QAbstractItemModel::modelReset, this, &TorrentsView::ModelChanged);
		connect(model, &AbstractTableItemModel::SortingChanged, this, &TorrentsView::OnSortingChanged);

		connect(m_tableView->horizontalHeader(), &QHeaderView::customContextMenuRequested,
		        this, &TorrentsView::OpenHeaderConfigurationWidget);

		if (m_sortColumn >= 0) m_model->sort(m_sortColumn, m_sortOrder);
		m_sortMenu = CreateSortMenu();
	}

	void TorrentsView::DisconnectModel()
	{
		delete m_headerModel;
		m_headerModel = nullptr;

		delete m_sortMenu;
		m_sortMenu = nullptr;

		m_tableView->setModel(nullptr);
		m_listView->setModel(nullptr);

		m_sizeHint = m_defMinSizeHint;
	}

	void TorrentsView::SetModel(std::shared_ptr<AbstractTableItemModel> model)
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

	void TorrentsView::InitHeaderTracking(QtTools::HeaderSectionInfoList * headerConf /* = nullptr */)
	{
		m_headerConfig = headerConf;
		if (not m_headerModel)
		{
			m_headerModel = new QtTools::HeaderControlModel(this);
			m_headerModel->Track(m_tableView->horizontalHeader());
		}

		if (m_headerConfig)
			m_headerModel->Configurate(*m_headerConfig);
	}

	TorrentsView::TorrentsView(QWidget * parent /* = nullptr */) : QFrame(parent)
	{
		setupUi();
		connectSignals();
		retranslateUi();

		SetViewMode(ListMode);
	}

	TorrentsView::~TorrentsView()
	{
		if (m_headerConfig)
			*m_headerConfig = m_headerModel->SaveConfiguration();
	}

	void TorrentsView::connectSignals()
	{
		//: filter shortcut in generic QTableView/QListView, shown as placeholder in QLineEdit
		auto * filterShortcut = new QShortcut(QKeySequence(tr("Ctrl+F")), this);
		connect(filterShortcut, &QShortcut::activated,
		        m_rowFilter, static_cast<void (QLineEdit::*)()>(&QLineEdit::setFocus));

		connect(m_rowFilter, &QLineEdit::textChanged, this, &TorrentsView::OnFilterChanged);
	}

	void TorrentsView::setupUi()
	{
		m_verticalLayout = new QVBoxLayout(this);
		m_rowFilter = new QLineEdit(this);
		m_rowFilter->setClearButtonEnabled(true);

		m_tableView = new QTableView(this);
		m_tableView->setSortingEnabled(true);
		m_tableView->setAlternatingRowColors(true);
		m_tableView->setTabKeyNavigation(false);

		m_tableView->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
		m_tableView->horizontalHeader()->setSectionsMovable(true);
		m_tableView->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
		m_tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
		//m_tableView->horizontalHeader()->setTextElideMode(Qt::ElideRight);

		m_tableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
		m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);

		m_nameDelegate = new QtTools::Delegates::SearchDelegate(this);
		m_tableDelegate = new FormattedDelegate(this);

		m_tableView->setItemDelegate(m_tableDelegate);

		auto * vertHeader = m_tableView->verticalHeader();
		vertHeader->setDefaultSectionSize(QtTools::CalculateDefaultRowHeight(m_tableView));
		vertHeader->hide();


		m_listView = new QListView(this);
		m_listView->setAlternatingRowColors(true);
		m_listView->setTabKeyNavigation(false);
		m_listView->setModelColumn(1);
		m_listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);

		m_listDelegate = new TorrentListDelegate(m_listView);
		m_listView->setItemDelegate(m_listDelegate);
		m_listView->installEventFilter(this);

		m_verticalLayout->addWidget(m_rowFilter);
		m_verticalLayout->addWidget(m_listView);
		m_verticalLayout->addWidget(m_tableView);
	}

	void TorrentsView::retranslateUi()
	{
		//: filter shortcut in generic QTableView/QListView, shown as placeholder in QLineEdit
		m_rowFilter->setPlaceholderText(tr("Row filter(Ctrl+F)"));
	}
}
