#include <qtor/FileTreeView.hqt>
#include <qtor/MainWindow.hqt>

#include <QtGui/QClipboard>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QMenu>

#include <QtTools/TableViewUtils.hpp>
#include <QtTools/HeaderConfigurationWidget.hqt>

#include <QtTools/ToolsBase.hpp>
#include <ext/join_into.hpp>

namespace qtor
{
	void FileTreeView::OnFilterChanged()
	{
		SetFilter(m_rowFilter->text());
	}

	void FileTreeView::SetFilter(QString newFilter)
	{
		m_filterString = std::move(newFilter);
		m_nameDelegate->SetFilterText(m_filterString);

		if (m_model)
		{
			m_model->FilterBy(m_filterString);
			m_treeView->viewport()->update();
		}
	}

	void FileTreeView::Sort(int column, Qt::SortOrder order)
	{
		if (not m_model) return;

		// model will emit signal and we will adjust ourselves in OnSortingChanged
		m_model->sort(m_sortColumn, m_sortOrder);
	}

	/************************************************************************/
	/*                Context Menu                                          */
	/************************************************************************/
	void FileTreeView::OpenHeaderConfigurationWidget()
	{
		if (not m_headerModel)
		{
			m_headerModel = new QtTools::HeaderControlModel(this);
			m_headerModel->Track(m_treeView->header());

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
	
	void FileTreeView::ViewSettings()
	{
		OpenHeaderConfigurationWidget();
	}
	
	void FileTreeView::ResizeColumnsToContents()
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
	
		// what we need is done by resizeColumnsToContents,
		// but it's also takes into account headers, and i want without them - have to do by hand
		m_treeView->resizeColumnToContents(0);
	
		auto * header = m_treeView->header();
		auto minimum = header->minimumSectionSize();
		int count = header->count();
	
		for (int i = 0; i < count; ++i)
		{
			auto li = header->logicalIndex(i);
			if (not header->isSectionHidden(i))
			{
				// for some unknown reason virtual method sizeHintForColumn is declared as protected in QTableView,
				// through it's public QAbstractItemView. call through base class
				auto hint = static_cast<QAbstractItemView *>(m_treeView)->sizeHintForColumn(li);
				header->resizeSection(li, std::max(hint, minimum));
			}
		}
		
		QApplication::restoreOverrideCursor();
	}

	//void FileTreeView::CopySelectedIntoClipboard()
	//{
	//	auto indexes = m_listView->selectionModel()->selectedRows();
	//	auto texts = indexes | boost::adaptors::transformed([this](auto & idx) { return m_listDelegate->GetText(idx); });

	//	QString text;
	//	auto sep = "\n" + QString(80, '-') + "\n";
	//	ext::join_into(texts, sep, text);

	//	qApp->clipboard()->setText(text);
	//}

//	QMenu * FileTreeView::CreateItemMenu(const QModelIndex & idx)
//	{
//		QAction * action;
//		auto * menu = new QMenu(this);
//		connect(menu, &QMenu::aboutToHide, menu, &QObject::deleteLater); // auto delete on hide
//
//		if (m_sortMenu)
//		{
//			menu->addMenu(m_sortMenu);
//			menu->addSeparator();
//		}
//
//		action = new QAction(tr("Table &settings..."), menu);
//		connect(action, &QAction::triggered, this, &FileTreeView::TableSettings);
//		menu->addAction(action);
//		
//		//: TableView context menu item
//		action = new QAction(tr("&Resize columns to content"), menu);
//		connect(action, &QAction::triggered, this, &FileTreeView::ResizeColumnsToContents);
//		menu->addAction(action);
//
//		menu->addSeparator();
//		action = new QAction(tr("&Table mode"), menu);
//		connect(action, &QAction::triggered, this, [this] { SetViewMode(TableMode); });
//		menu->addAction(action);
//
//		action = new QAction(tr("&List mode"), menu);
//		connect(action, &QAction::triggered, this, [this] { SetViewMode(ListMode); });
//		menu->addAction(action);
//
////		if (idx.isValid())
////		{
////			QPersistentModelIndex pidx = idx;
////
////#define CONNECT(METHOD)                                                                     \
////			action->setData(pidx);                                                          \
////			connect(action, &QAction::triggered, this, [this]                               \
////			{                                                                               \
////				auto * action = static_cast<QAction *>(QObject::sender());                  \
////				QModelIndex idx = qvariant_cast<QPersistentModelIndex>(action->data());     \
////				m_parent->METHOD(idx);                                                      \
////			})                                                                              \
////
////			action = menu->addAction(tr("&Properties"));
////			action->setIcon(QIcon::fromTheme("document-properties"));
////			action->setShortcut(QKeySequence("Alt+Enter"));
////			CONNECT(OpenTorrentLocationSettings);
////			
////			action = menu->addAction(tr("Open Fold&er"));
////			action->setIcon(QIcon::fromTheme("folder-open"));
////			action->setShortcut(QKeySequence("Alt+E"));
////			CONNECT(OpenTorrentFolder);
////
////			action = menu->addAction(tr("Start"));
////			CONNECT(StartTorrent);
////			
////			action = menu->addAction(tr("start Now"));
////			CONNECT(StartTorrentNow);
////			
////			action = menu->addAction(tr("Stop"));
////			CONNECT(StopTorrent);
////
////			action = menu->addAction(tr("Announce"));
////			CONNECT(AnnounceTorrent);
////
////			action = menu->addAction(tr("Remove"));
////			CONNECT(DeleteTorrent);
////
////			action = menu->addAction(tr("Remove and Delete data"));
////			CONNECT(PurgeTorrent);
////
////
////
////#undef CONNECT
////		}
//
//		return menu;
//	}
//
//	void FileTreeView::contextMenuEvent(QContextMenuEvent * ev)
//	{
//		ev->accept();
//		auto pos = ev->globalPos();
//
//		auto * viewport = m_itemView->viewport();
//		auto viewPos = viewport->mapFromGlobal(pos);
//
//		// process only menu from QTableView
//		if (not viewport->contentsRect().contains(viewPos))
//			return;
//
//		auto idx = m_treeView->indexAt(viewPos);
//		auto * menu = CreateItemMenu(idx);
//		if (menu) menu->popup(pos);
//	}

	QSize FileTreeView::sizeHint() const
	{
		return base_type::sizeHint();

		//// maximum size - half screen
		//QSize maxSize = QApplication::desktop()->screenGeometry().size();
		//maxSize /= 2;
		//// but no more than maximumSize()
		//maxSize = maxSize.boundedTo(maximumSize());

		//// if we are in QMdiArea, then our sizeHint should not be more than one third of QMdiArea size
		//if (auto mdi = QtTools::FindAncestor<QMdiArea>(this))
		//{
		//	maxSize = mdi->size();
		//	maxSize /= 3;
		//}

		//// additional size - size of all layout'à
		//// minus size of tableView, size of which we calculate ourself.
		//// QTableView::sizeHint in fact always reutrn dummy size: 256:192
		//QSize addSz = QWidget::sizeHint() - m_treeView->sizeHint();
		//maxSize -= addSz;

		//auto sz = QtTools::TableSizeHint(m_treeView, m_sizeHint, maxSize);
		//return sz += addSz;
	}

	//bool FileTreeView::eventFilter(QObject * watched, QEvent * event)
	//{
	//	if (event->type() == QEvent::KeyPress)
	//	{
	//		auto * keyEvent = static_cast<QKeyEvent *>(event);
	//		if (keyEvent->matches(QKeySequence::Copy) and m_listView == watched)
	//		{
	//			CopySelectedIntoClipboard();
	//			return true;
	//		}
	//	}

	//	return false;
	//}

	/************************************************************************/
	/*                    Init Methods                                      */
	/************************************************************************/
	void FileTreeView::ConnectModel()
	{
		auto * model = m_model.get();
		m_treeView->setModel(model);
		m_rowFilter->clear();
		
		//int nameCol = model->FindColumn(torrent::Name);
		//m_treeView->setItemDelegateForColumn(nameCol, m_nameDelegate);
		m_treeView->setItemDelegateForColumn(0, m_nameDelegate);

		//connect(model, &QAbstractItemModel::layoutChanged, this, &FileTreeView::ModelChanged);
		//connect(model, &QAbstractItemModel::modelReset, this, &FileTreeView::ModelChanged);
		//connect(model, &AbstractSparseContainerModel::SortingChanged, this, &FileTreeView::OnSortingChanged);

		connect(m_treeView->header(), &QHeaderView::customContextMenuRequested,
		        this, &FileTreeView::OpenHeaderConfigurationWidget);

		if (m_sortColumn >= 0) m_model->sort(m_sortColumn, m_sortOrder);
		//m_sortMenu = CreateSortMenu();
	}

	void FileTreeView::DisconnectModel()
	{
		delete m_headerModel;
		m_headerModel = nullptr;

		//delete m_sortMenu;
		//m_sortMenu = nullptr;

		m_treeView->setModel(nullptr);

		m_sizeHint = m_defMinSizeHint;
	}

	void FileTreeView::SetModel(std::shared_ptr<FileTreeModel> model)
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

	void FileTreeView::InitHeaderTracking(QtTools::HeaderSectionInfoList * headerConf /* = nullptr */)
	{
		m_headerConfig = headerConf;
		if (not m_headerModel)
		{
			m_headerModel = new QtTools::HeaderControlModel(this);
			m_headerModel->Track(m_treeView->header());
		}

		if (m_headerConfig)
			m_headerModel->Configurate(*m_headerConfig);
	}

	FileTreeView::FileTreeView(QWidget * parent /* = nullptr */) : QFrame(parent)
	{
		setupUi();
		connectSignals();
		retranslateUi();
	}

	FileTreeView::~FileTreeView()
	{
		if (m_headerConfig)
			*m_headerConfig = m_headerModel->SaveConfiguration();
	}

	void FileTreeView::connectSignals()
	{
		//: filter shortcut in generic QTableView/QListView, shown as placeholder in QLineEdit
		auto * filterShortcut = new QShortcut(QKeySequence(tr("Ctrl+F")), this);
		connect(filterShortcut, &QShortcut::activated,
		        m_rowFilter, static_cast<void (QLineEdit::*)()>(&QLineEdit::setFocus));

		connect(m_rowFilter, &QLineEdit::textChanged, this, &FileTreeView::OnFilterChanged);
	}

	void FileTreeView::setupUi()
	{
		m_verticalLayout = new QVBoxLayout(this);
		m_rowFilter = new QLineEdit(this);
		m_rowFilter->setClearButtonEnabled(true);

		m_treeView = new QTreeView(this);
		m_treeView->setSortingEnabled(true);
		m_treeView->setAlternatingRowColors(true);
		m_treeView->setTabKeyNavigation(false);

		m_treeView->header()->setSortIndicator(-1, Qt::AscendingOrder);
		//m_treeView->header()->setSectionsMovable(true);
		m_treeView->header()->setContextMenuPolicy(Qt::CustomContextMenu);
		m_treeView->header()->setDefaultAlignment(Qt::AlignLeft);
		//m_treeView->header()->setTextElideMode(Qt::ElideRight);

		m_treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
		m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

		m_nameDelegate = new QtTools::Delegates::SearchDelegate(this);

		auto * vertHeader = m_treeView->header();
		//vertHeader->setDefaultSectionSize(QtTools::CalculateDefaultRowHeight(m_treeView));
		vertHeader->setDefaultSectionSize(21);
		vertHeader->hide();		

		m_verticalLayout->addWidget(m_rowFilter);
		m_verticalLayout->addWidget(m_treeView);
	}

	void FileTreeView::retranslateUi()
	{
		//: filter shortcut in generic QTableView/QListView, shown as placeholder in QLineEdit
		m_rowFilter->setPlaceholderText(tr("Row filter(Ctrl+F)"));
	}
}
