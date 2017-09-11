#include <qtor/TorrentTableWidget.hqt>
#include <QtWidgets/QShortcut>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QMdiSubWindow>

#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>

#include <QtTools/HeaderConfigurationWidget.hqt>
#include <QtTools/TableViewUtils.hpp>

namespace qtor
{
	void TorrentTableWidget::OnFilterChanged()
	{
		SetFilter(m_rowFilter->text());
	}

	void TorrentTableWidget::SetFilter(QString newFilter)
	{
		m_filterString = std::move(newFilter);
		//m_delegate->SetSearchWord(m_filterString);

		if (m_model) m_model->FilterBy(m_filterString);
	}

	QSize TorrentTableWidget::sizeHint() const
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

		auto sz = QtTools::TableSizeHint(m_tableView, m_sizeHint, maxSize);
		return sz += addSz;
	}

	void TorrentTableWidget::contextMenuEvent(QContextMenuEvent * ev)
	{
		ev->accept();
		auto pos = ev->globalPos();

		auto * viewport = m_tableView->viewport();
		auto viewPos = viewport->mapFromGlobal(pos);

		// обрабатываем только меню по QTableView
		if (not viewport->contentsRect().contains(viewPos))
			return;

		auto idx = m_tableView->indexAt(viewPos);
		//auto * menu = CreateItemMenu(idx);
		//if (menu) menu->popup(pos);
	}

	/************************************************************************/
	/*                Context Menu                                          */
	/************************************************************************/
	void TorrentTableWidget::OpenHeaderConfigurationWidget()
	{
		auto * confWgt = findChild<QtTools::HeaderConfigurationWidget *>("ConfigurationWidget");
		if (confWgt && confWgt->isVisible()) {
			confWgt->activateWindow();
			return;
		}
	
		delete confWgt;
		confWgt = new QtTools::HeaderConfigurationWidget(*m_headerModel, this);
		confWgt->setAttribute(Qt::WA_DeleteOnClose);
		confWgt->setObjectName("ConfigurationWidget");
		confWgt->show();
		confWgt->activateWindow();
	}
	
	void TorrentTableWidget::TableSettings()
	{
		OpenHeaderConfigurationWidget();
	}
	
	void TorrentTableWidget::ResizeColumnsToContents()
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
	
		// what we need is done by resizeColumnsToContents,
		// but it's also takes into account headers, and i want without them - have to do by hand
		//m_tableView->resizeColumnsToContents();
	
		auto * header = m_tableView->horizontalHeader();
		auto minimum = header->minimumSectionSize();
		int count = header->count();
	
		for (int i = 0; i < count; ++i)
		{
			auto li = header->logicalIndex(i);
			if (not header->isSectionHidden(i))
			{
				// for some unknown reason virtual method sizeHintForColumn is declared as protected in QTableView,
				// through it's public QAbstractItemView. call thruogh base class
				auto hint = static_cast<QAbstractItemView *>(m_tableView)->sizeHintForColumn(li);
				header->resizeSection(li, std::max(hint, minimum));
			}
		}
		
		QApplication::restoreOverrideCursor();
	}

	/************************************************************************/
	/*                    Init Methods                                      */
	/************************************************************************/
	void TorrentTableWidget::ConnectModel()
	{
		auto * model = m_model.get();
		m_tableView->setModel(model);
		m_rowFilter->clear();


		//connect(model, &QAbstractItemModel::layoutChanged, this, &TorrentTableWidget::ModelChanged);
		//connect(model, &QAbstractItemModel::modelReset, this, &TorrentTableWidget::ModelChanged);

		connect(m_tableView->horizontalHeader(), &QHeaderView::customContextMenuRequested,
		        this, &TorrentTableWidget::OpenHeaderConfigurationWidget);
	}

	void TorrentTableWidget::DisconnectModel()
	{
		delete m_headerModel;
		m_headerModel = nullptr;
		m_tableView->setModel(nullptr);

		m_sizeHint = m_defMinSizeHint;
	}

	void TorrentTableWidget::Init(std::shared_ptr<AbstractSparseContainerModel> model)
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

	void TorrentTableWidget::InitHeaderTracking(QtTools::HeaderSectionInfoList * headerConf /* = nullptr */)
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

	TorrentTableWidget::TorrentTableWidget(QWidget * parent /* = nullptr */) : QFrame(parent)
	{
		setupUi();
		connectSignals();
		retranslateUi();
	}

	TorrentTableWidget::~TorrentTableWidget()
	{
		if (m_headerConfig)
			*m_headerConfig = m_headerModel->SaveConfiguration();
	}

	void TorrentTableWidget::connectSignals()
	{
		//: filter shortcut in generic BasicTableWidget, probably should not be translated
		auto * filterShortcut = new QShortcut(QKeySequence(tr("Ctrl+F")), this);
		connect(filterShortcut, &QShortcut::activated,
		        m_rowFilter, static_cast<void (QLineEdit::*)()>(&QLineEdit::setFocus));

		connect(m_rowFilter, &QLineEdit::textChanged, this, &TorrentTableWidget::OnFilterChanged);
	}

	void TorrentTableWidget::setupUi()
	{
		m_verticalLayout = new QVBoxLayout(this);
		m_rowFilter = new QLineEdit(this);
		m_rowFilter->setClearButtonEnabled(true);

		m_tableView = new QTableView(this);
		m_tableView->setSortingEnabled(true);
		m_tableView->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
		m_tableView->horizontalHeader()->setSectionsMovable(true);
		m_tableView->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
		m_tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
		m_tableView->setTabKeyNavigation(false);
		//m_tableView->horizontalHeader()->setTextElideMode(Qt::ElideRight);

		m_tableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
		m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);

		auto * vertHeader = m_tableView->verticalHeader();
		vertHeader->setDefaultSectionSize(QtTools::CalculateDefaultRowHeight(m_tableView));
		vertHeader->hide();

		m_verticalLayout->addWidget(m_rowFilter);
		m_verticalLayout->addWidget(m_tableView);

		setFocusProxy(m_tableView);
		QWidget::setTabOrder(m_rowFilter, m_tableView);
	}

	void TorrentTableWidget::retranslateUi()
	{
		//: filter shortcut in generic BasicTableWidget, shown as placeholder in EditWidget
		m_rowFilter->setPlaceholderText(tr("Row filter(Ctrl+F)"));
	}
}
