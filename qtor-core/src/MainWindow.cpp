#include <qtor/MainWindow.hqt>

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
	}

	MainWindow::MainWindow(QWidget * wgt /*= nullptr*/) : QMainWindow(wgt)
	{
		setupUi();
		connectSignals();
		retranslateUi();
	}

	MainWindow::~MainWindow()
	{

	}

	void MainWindow::setupToolBars()
	{

	}

	void MainWindow::setupStatusBar()
	{

	}

	void MainWindow::connectSignals()
	{

	}

	void MainWindow::setupUi()
	{
		m_torrentWidget = new TorrentTableWidget(this);
		setCentralWidget(m_torrentWidget);
	}

	void MainWindow::retranslateUi()
	{

	}

	QPixmap MainWindow::ScalePixmap(const QPixmap & pcx)
	{
		auto height = std::min(m_statusBarLabel->height(), 16);
		return pcx.scaledToHeight(height, Qt::SmoothTransformation);
	}

	void MainWindow::SetConnectedStatus()
	{
		m_statusBarLabel->setPixmap(ScalePixmap({":/icons/connect_status/connected.ico"}));
	}

	void MainWindow::SetDisconnectedStatus()
	{
		m_statusBarLabel->setPixmap(ScalePixmap({":/icons/connect_status/disconnected.ico"}));
	}

}
