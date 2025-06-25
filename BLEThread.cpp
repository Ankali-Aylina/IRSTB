#include "BLEThread.h"

// 静态函数指针初始化
static CreateBleHandleFunc s_createHandle = nullptr;
static DestroyBleHandleFunc s_destroyHandle = nullptr;
static StartBleScanFunc s_startScan = nullptr;
static StopBleScanFunc s_stopScan = nullptr;
static ConnectBleDeviceFunc s_connectDevice = nullptr;
static DisconnectBleDeviceFunc s_disconnectDevice = nullptr;
static CheckCharacteristicWritePermissionfunc s_checkCharacteristicWritePermission = nullptr;
static WriteBleCharacteristicFunc s_writeDataByCharacteristic = nullptr;
static IsConnectedFunc s_isConnected = nullptr;

QString g_name;
QString g_deviceId;
bool g_isFind = false;

void OnDeviceScannedStatic(void* device, const char* name, const char* address, const char* deviceId, int rssi)
{
	qDebug() << "Device scanned:" << name << address << deviceId << rssi;
	if (name != nullptr && (QString::fromUtf8(name) == g_name))
	{
		g_deviceId = deviceId;
		g_isFind = true;
	}
}

void scanVerifyDeviceStatic(void* device, const char* name, const char* address, const char* deviceId, int rssi)
{
	qDebug() << "Device scanned:" << name << address << deviceId << rssi;
	if (deviceId != nullptr && (QString::fromUtf8(deviceId) == g_deviceId)) {
		g_isFind = true;
	}
}

BLEThread::BLEThread(QObject* parent)
	: QObject(parent), m_logger(new LogManagement(this)) {
	connect(this, &BLEThread::logMessage, m_logger, &LogManagement::logMessage);
}

BLEThread::~BLEThread() {
	safeUnloadLibrary();
}

bool BLEThread::loadBleLibrary()
{
	if (m_dllState == DllState::Loaded) return true;

	m_bleLibrary.setFileName("BLEDll.dll");
	if (!m_bleLibrary.load()) {
		emit logMessage(QString("BLE DLL库加载失败: " + m_bleLibrary.errorString()),
			LogManagement::LOG_ERROR);
		m_dllState = DllState::Failed;
		return false;
	}

	// 使用类型修正后的 lambda
	auto resolve = [this](auto& func, const char* name) {
		using FuncType = std::remove_reference_t<decltype(func)>;
		func = reinterpret_cast<FuncType>(m_bleLibrary.resolve(name));
		return func != nullptr;
		};

	bool success = resolve(s_createHandle, "CreateBleHandle") &&
		resolve(s_destroyHandle, "DestroyBleHandle") &&
		resolve(s_startScan, "StartBleScanWrapper") &&
		resolve(s_stopScan, "StopBleScanWrapper") &&
		resolve(s_connectDevice, "ConnectBLEDeviceWrapper") &&
		resolve(s_disconnectDevice, "DisconnectBLEDeviceWrapper") &&
		resolve(s_checkCharacteristicWritePermission, "CheckCharacteristicWritePermissionWrapper") &&
		resolve(s_writeDataByCharacteristic, "WriteDataByCharacteristicWrapper") &&
		resolve(s_isConnected, "IsConnectedWrapper");

	if (!success) {
		emit logMessage(QString("BLE Dll库函数解析失败"),
			LogManagement::LOG_ERROR);
		safeUnloadLibrary();
		return false;
	}

	m_dllState = DllState::Loaded;
	return true;
}

void BLEThread::run()
{
	if (loadBleLibrary()) {
		initializeBle();
	}
}

void BLEThread::reconnectDevice()
{
	if (m_deviceInfo.isFind) {
		if (m_deviceInfo.isConnected) {
			s_disconnectDevice(m_bleHandle);
			m_deviceInfo.isConnected = false;
		}
	}

	safeUnloadLibrary();

	if (loadBleLibrary()) {
		initializeBle();
	}
}

void BLEThread::updateConnectionStatus()
{
	ConnectionStatus();
	if (!m_deviceInfo.isConnected) {
		emit disconnected();
	}
}

void BLEThread::controlFan(char* buff)
{
	QMutexLocker lock(&m_fanControlMutex);

	if (!m_deviceInfo.isFind)return;
	ConnectionStatus();

	if (m_deviceInfo.isConnected)
	{
		sendData(reinterpret_cast<const unsigned char*>(buff));
		emit logMessage(QString("发送数据成功:") + buff, LogManagement::LogLevel::LOG_INFO);
	}
	else
	{
		emit logMessage(QString("BLE连接断开"), LogManagement::LogLevel::LOG_WARNING);
		emit disconnected();
	}
	return;
}

void BLEThread::autoMode()
{
	QMutexLocker lock(&m_modeMutex);

	if (!m_deviceInfo.isFind)return;
	ConnectionStatus();

	if (m_deviceInfo.isConnected)
	{
		int value = 0;
		static char buffer[2]; // 静态缓冲区避免悬空指针
		snprintf(buffer, sizeof(buffer), "%d", value);
		sendData(reinterpret_cast<const unsigned char*>(buffer));
		emit logMessage(QString("自动模式启动"), LogManagement::LogLevel::LOG_INFO);
		memset(buffer, 0, sizeof(buffer));
	}
	else
	{
		emit logMessage(QString("BLE连接断开"), LogManagement::LogLevel::LOG_WARNING);
		emit disconnected();
	}

	return;
}

void BLEThread::silentMode()
{
	QMutexLocker lock(&m_modeMutex);

	if (!m_deviceInfo.isFind)return;
	ConnectionStatus();

	if (m_deviceInfo.isConnected)
	{
		int value = 5;
		static char buffer[2]; // 静态缓冲区避免悬空指针
		snprintf(buffer, sizeof(buffer), "%d", value);
		sendData(reinterpret_cast<const unsigned char*>(buffer));
		emit logMessage(QString("静音模式启动"), LogManagement::LogLevel::LOG_INFO);
		memset(buffer, 0, sizeof(buffer));
	}
	else
	{
		emit logMessage(QString("BLE连接断开"), LogManagement::LogLevel::LOG_WARNING);
		emit disconnected();
	}

	return;
}

void BLEThread::performanceMode()
{
	QMutexLocker lock(&m_modeMutex);

	if (!m_deviceInfo.isFind)return;
	ConnectionStatus();

	if (m_deviceInfo.isConnected)
	{
		int value = 6;
		static char buffer[2]; // 静态缓冲区避免悬空指针
		snprintf(buffer, sizeof(buffer), "%d", value);
		sendData(reinterpret_cast<const unsigned char*>(buffer));
		emit logMessage(QString("全速模式启动"), LogManagement::LogLevel::LOG_INFO);
		memset(buffer, 0, sizeof(buffer));
	}
	else
	{
		emit logMessage(QString("BLE连接断开"), LogManagement::LogLevel::LOG_WARNING);
		emit disconnected();
	}

	return;
}

void BLEThread::safeUnloadLibrary() {
	if (m_bleHandle) {
		s_destroyHandle(m_bleHandle);
		m_bleHandle = nullptr;
	}
	if (m_bleLibrary.isLoaded()) {
		m_bleLibrary.unload();
	}
	m_dllState = DllState::NotLoaded;
}

void BLEThread::initializeIniFile()
{
	IniManagement::instance().IniFileData.IniFile_Status = 0;

	IniManagement::instance().InitSection("BLE", "false");
	IniManagement::instance().write("BLE", "Init", "false");
	IniManagement::instance().write("BLE", "TargetName", "BT24-T");
	IniManagement::instance().write("BLE", "TargetServiceUUID", 0xFFE0);
	IniManagement::instance().write("BLE", "TargetCharacteristicUUID", 0xFFE1);

	IniManagement::instance().IniFileData.IniFile_Status = 1;

	IniManagement::instance().InitSection("BLE", "true");

	emit logMessage(QString("BLE配置文件创建并写入完成"), LogManagement::LogLevel::LOG_INFO);
}

void BLEThread::loadDeviceConfig()
{
	m_deviceInfo.name = IniManagement::instance().read("BLE", "TargetName").toString();
	m_deviceInfo.serviceUuid = IniManagement::instance().read("BLE", "TargetServiceUUID").toUInt();
	m_deviceInfo.characteristicUuid = IniManagement::instance().read("BLE", "TargetCharacteristicUUID").toUInt();
	g_name = m_deviceInfo.name;
}

void BLEThread::scanDevices()
{
	emit bleScanStarted(); // 发送扫描开始信号
	QEventLoop loop;
	QTimer timer, timer_maxtime;
	timer.setSingleShot(false);
	timer_maxtime.setSingleShot(true);
	connect(&timer, &QTimer::timeout, [&]() {
		if (g_isFind)
		{
			m_deviceInfo.id = g_deviceId;
			m_deviceInfo.isFind = g_isFind;

			timer.stop();
			timer_maxtime.stop();
			loop.quit();
		}
		});

	connect(&timer_maxtime, &QTimer::timeout, [&]() {
		timer.stop();
		timer_maxtime.stop();
		loop.quit();
		});

	timer.start(1000); // 设置定时器，每隔1秒检查一次
	timer_maxtime.start(10000); // 设置最大扫描时间，10秒后停止扫描
	s_startScan(m_bleHandle, OnDeviceScannedStatic);

	loop.exec();

	s_stopScan(m_bleHandle);
}

void BLEThread::scanVerifyDevice()
{
	emit bleScanStarted(); // 发送扫描开始信号
	QEventLoop loop;
	QTimer timer, timer_maxtime;
	timer.setSingleShot(false);
	timer_maxtime.setSingleShot(true);
	connect(&timer, &QTimer::timeout, [&]() {
		if (g_isFind)
		{
			m_deviceInfo.isFind = g_isFind;

			timer.stop();
			timer_maxtime.stop();
			loop.quit();
		}
		});

	connect(&timer_maxtime, &QTimer::timeout, [&]() {
		timer.stop();
		timer_maxtime.stop();
		loop.quit();
		});

	timer.start(1000); // 设置定时器，每隔1秒检查一次
	timer_maxtime.start(10000); // 设置最大扫描时间，10秒后停止扫描
	s_startScan(m_bleHandle, scanVerifyDeviceStatic);

	loop.exec();

	s_stopScan(m_bleHandle);
}

void BLEThread::initializeBle() {
	{
		QMutexLocker lock(&m_operationMutex);

		m_bleHandle = s_createHandle();
		if (!m_bleHandle) {
			emit logMessage(QString("BLE句柄创建失败"), LogManagement::LOG_ERROR);
			return;
		}

		if (!IniManagement::instance().IsExist()) {
			initializeIniFile();
		}

		if (!(IniManagement::instance().IsInit("BLE")))
		{
			initializeIniFile();
		}

		loadDeviceConfig();
	}
	if (IniManagement::instance().read("BLE", "Init").toString() == "false") {
		performFirstConnection();
		return;
	}
	else {
		connectToDevice(IniManagement::instance().read("BLE", "TargetID").toString());
		return;
	}
}

void BLEThread::performFirstConnection()
{
	scanDevices();

	{
		QMutexLocker lock(&m_operationMutex);
		if (!m_deviceInfo.isFind)
		{
			emit logMessage(QString("连接超时！"), LogManagement::LogLevel::LOG_WARNING);
			m_deviceInfo.retryCount++;
			if (m_deviceInfo.retryCount < 3)
			{
				performFirstConnection();
				return;
			}
			else
			{
				emit logMessage(QString("连接失败！已超过最大重连次数！"), LogManagement::LogLevel::LOG_ERROR);
				bleScanTimeout();//扫描超时信号
				return;
			}
		}

		emit connectionInProgress();

		if (m_deviceInfo.isFind && m_deviceInfo.id != nullptr) {
			if (s_connectDevice(m_bleHandle, m_deviceInfo.id.toUtf8().constData())) {
				IniManagement::instance().write("BLE", "TargetID", m_deviceInfo.id);
				IniManagement::instance().write("BLE", "Init", "true");
				emit logMessage(QString("连接成功！"), LogManagement::LogLevel::LOG_INFO);
				m_deviceInfo.isConnected = true;
				emit connected();
			}
			else {
				emit logMessage(QString("连接失败！"), LogManagement::LogLevel::LOG_ERROR);
				emit connectionFailed();
			}
		}
	}
}

void BLEThread::connectToDevice(const QString& deviceId)
{
	g_deviceId = deviceId;

	scanVerifyDevice();

	{
		QMutexLocker lock(&m_operationMutex);
		if (!m_deviceInfo.isFind)
		{
			emit logMessage(QString("连接超时！"), LogManagement::LogLevel::LOG_WARNING);
			m_deviceInfo.retryCount++;
			if (m_deviceInfo.retryCount < 3)
			{
				connectToDevice(deviceId);
				return;
			}
			else
			{
				emit logMessage(QString("连接失败！已超过最大重连次数！"), LogManagement::LogLevel::LOG_ERROR);
				bleScanTimeout();//扫描超时信号
				return;
			}
		}

		emit connectionInProgress();

		if (s_connectDevice(m_bleHandle, deviceId.toUtf8().constData())) {
			emit logMessage(QString("连接成功！"), LogManagement::LogLevel::LOG_INFO);
			m_deviceInfo.isConnected = true;
			emit connected();
		}
		else {
			emit logMessage(QString("连接失败！"), LogManagement::LogLevel::LOG_ERROR);
			emit connectionFailed();
		}
	}
}

void BLEThread::ConnectionStatus()
{
	m_deviceInfo.isConnected = s_isConnected(m_bleHandle);
}

void BLEThread::sendData(const unsigned char* data)
{
	if (m_deviceInfo.isConnected) {
		if (s_writeDataByCharacteristic(m_bleHandle, m_deviceInfo.serviceUuid, m_deviceInfo.characteristicUuid, data, sizeof(data), true)) {
			emit logMessage(QString("发送成功！"), LogManagement::LogLevel::LOG_INFO);
		}
	}
	else
	{
		emit logMessage(QString("数据发送失败！"), LogManagement::LogLevel::LOG_ERROR);
	}
}