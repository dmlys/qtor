#include <qtor/MainWindow.hqt>
#include <QtTools/Utility.hpp>

#include <QtWidgets/QFileDialog>
#include <QtSvg/QSvgGenerator>

namespace qtor
{
	void MainWindow::OnDisconnected()
	{
		SetDisconnectedStatus();
	}

	void MainWindow::OnConnected()
	{
		SetConnectedStatus();
	}

	void MainWindow::OnConnectionError()
	{

	}

	void MainWindow::Connect()
	{
		m_app->Connect();
	}

	void MainWindow::Disconnect()
	{
		m_app->Disconnect();
	}

	void MainWindow::Init(Application & app)
	{
		m_app = &app;

		connect(m_app, &Application::Connected, this, &MainWindow::OnConnected);
		connect(m_app, &Application::Disconnected, this, &MainWindow::OnDisconnected);
		connect(m_app, &Application::ConnectionError, this, &MainWindow::OnConnectionError);

		auto model = m_app->AccquireTorrentModel();
		m_torrentWidget->SetModel(std::move(model));
		m_torrentWidget->InitHeaderTracking(nullptr);
	}

	MainWindow::MainWindow(QWidget * wgt /*= nullptr*/) : QMainWindow(wgt)
	{
		setupUi();
		setupStatusBar();
		setupToolBars();
		setupMenu();

		connectSignals();

		retranslateUi();
		retranslateToolBars();
		retranslateStatusBar();
	}

	MainWindow::~MainWindow()
	{

	}

	void MainWindow::setupToolBars()
	{
		m_actionOpen = new QAction(this);
		m_actionStartAll = new QAction(this);
		m_actionStopAll = new QAction(this);
		m_actionDelete = new QAction(this);
		m_actionPreferences = new QAction(this);

		m_toolBar = new QToolBar(this);
		m_toolBar->setObjectName("main_toolbar");
		m_toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

		m_toolBar->addAction(m_actionOpen);
		m_toolBar->addAction(m_actionStartAll);
		m_toolBar->addAction(m_actionStopAll);
		m_toolBar->addAction(m_actionDelete);
		m_toolBar->addAction(m_actionPreferences);


		addToolBar(Qt::TopToolBarArea, m_toolBar);
	}

	void MainWindow::setupStatusBar()
	{
		m_statusbar = new QStatusBar(this);
		m_statusBarLabel = new QLabel(this);
		m_statusbar->addWidget(m_statusBarLabel);
		setStatusBar(m_statusbar);
	}

	void MainWindow::setupUi()
	{
		m_torrentWidget = new TorrentsView(this);
		setCentralWidget(m_torrentWidget);
	}

	void MainWindow::setupMenu()
	{
		QMenu * main = menuBar()->addMenu(tr("&Main"));
		main->addAction(m_actionOpen);
		main->addAction(m_actionStartAll);
		main->addAction(m_actionStopAll);
		main->addAction(m_actionDelete);
		main->addAction(m_actionPreferences);
	}

	void MainWindow::connectSignals()
	{
		connect(m_actionOpen, &QAction::triggered, this, &MainWindow::saveToSvg);
	}

	void MainWindow::saveToSvg()
	{
		QString path = QFileDialog::getSaveFileName(this, tr("Save SVG"));

		if (path.isEmpty()) return;

		QSvgGenerator generator;
		generator.setFileName(path);
		auto dpi = this->logicalDpiX();
		generator.setResolution(dpi);
		//generator.setDescription()

		this->render(&generator);

	}

	void MainWindow::retranslateUi()
	{

	}

	void MainWindow::retranslateToolBars()
	{
		m_toolBar->setWindowTitle(tr("Main Toolbar"));

		m_actionOpen->setIcon(QtTools::LoadIcon(QStringLiteral("document-open"), QStyle::SP_DialogOpenButton));
		m_actionOpen->setText(tr("&Open"));
		m_actionOpen->setToolTip("Open(Ctrl+N)");
		m_actionOpen->setShortcut(tr("Ctrl+N"));

		m_actionStartAll->setIcon(QtTools::LoadIcon(QStringLiteral("media-playback-start"), QStyle::SP_MediaPlay));
		m_actionStartAll->setText(tr("&Start all"));
		m_actionStartAll->setToolTip(tr("Start all(Ctrl+S)"));
		m_actionStartAll->setShortcut(tr("Ctrl+S"));

		m_actionStopAll->setIcon(QtTools::LoadIcon(QStringLiteral("media-playback-pause"), QStyle::SP_MediaStop));
		m_actionStopAll->setText(tr("Sto&p all"));
		m_actionStopAll->setToolTip(tr("Stop all(Ctrl+P)"));
		m_actionStopAll->setShortcut(tr("Ctrl+P"));

		m_actionDelete->setIcon(QtTools::LoadIcon(QStringLiteral("list-remove"), QStyle::SP_TrashIcon));
		m_actionDelete->setText(tr("&Delete"));
		//m_actionDelete->setShortcut(tr("Ctrl+D"));

		m_actionPreferences->setIcon(QtTools::LoadIcon(QStringLiteral("preferences-system"), QStringLiteral(":/icons/qtor-core/preferences.png")));
		m_actionPreferences->setText(tr("Pre&ferences"));
		m_actionPreferences->setToolTip(tr("Preferences(Ctrl+P)"));
		m_actionPreferences->setShortcut(tr("Ctrl+P"));
	}

	void MainWindow::retranslateStatusBar()
	{
		m_connectedPixmap = ScalePixmap({":/icons/qtor-core/connected.ico"});
		m_disconnectedPixmap = ScalePixmap({":/icons/qtor-core/disconnected.ico"});
	}

	QPixmap MainWindow::ScalePixmap(const QPixmap & pcx)
	{
		auto height = std::min(m_statusBarLabel->height(), 16);
		return pcx.scaledToHeight(height, Qt::SmoothTransformation);
	}

	void MainWindow::SetConnectedStatus()
	{
		m_statusBarLabel->setPixmap(m_connectedPixmap);
	}

	void MainWindow::SetDisconnectedStatus()
	{
		m_statusBarLabel->setPixmap(m_disconnectedPixmap);
	}

}
