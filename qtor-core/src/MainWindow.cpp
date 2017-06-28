#include <qtor/MainWindow.hqt>

namespace qtor
{
	void MainWindow::OnDisconnected()
	{

	}

	void MainWindow::OnConnected()
	{

	}

	void MainWindow::OnConnectionError(QString errMsg)
	{

	}

	void MainWindow::Connect()
	{

	}

	void MainWindow::Disconnect()
	{

	}

	void MainWindow::Init(Application & app)
	{

	}

	MainWindow::MainWindow(QWidget * wgt /*= nullptr*/)
	{

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
