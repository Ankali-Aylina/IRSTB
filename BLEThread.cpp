#include "BLEThread.h"
#include <QDebug>

// 唯一全局：当前正在扫描的 BLEThread 实例指针（DLL 回调无 userData，通过此路由）
static std::atomic<BLEThread*> s_activeScanner{ nullptr };

// RAII 守卫：确保 s_activeScanner 在任何退出路径下都被重置
class ScannerGuard {
public:
	explicit ScannerGuard(BLEThread* scanner)
		{ s_activeScanner.store(scanner, std::memory_order_release); }
	~ScannerGuard()
		{ s_activeScanner.store(nullptr, std::memory_order_release); }
	ScannerGuard(const ScannerGuard&) = delete;
	ScannerGuard& operator=(const ScannerGuard&) = delete;
};

// DLL 回调：通过 s_activeScanner 路由到正确的 BLEThread 实例
void bleDeviceScannedCallback(void* device, const char* name, const char* address, const char* deviceId, int rssi)
{
	auto* self = s_activeScanner.load(std::memory_order_acquire);
	if (!self) return;

	qDebug() << "Device scanned:" << name << address << deviceId << rssi;
	auto& ctx = self->m_scanCtx;
	if (name != nullptr && (QString::fromUtf8(name) == ctx.targetName))
	{
		ctx.foundDeviceId = QString::fromUtf8(deviceId ? deviceId : "");
		ctx.isFound.store(true, std::memory_order_release);
	}
}

void bleVerifyDeviceCallback(void* device, const char* name, const char* address, const char* deviceId, int rssi)
{
	auto* self = s_activeScanner.load(std::memory_order_acquire);
	if (!self) return;

	qDebug() << "Device scanned:" << name << address << deviceId << rssi;
	auto& ctx = self->m_scanCtx;
	if (deviceId != nullptr && (QString::fromUtf8(deviceId) == ctx.foundDeviceId))
	{
		ctx.isFound.store(true, std::memory_order_release);
	}
}

BLEThread::BLEThread(IConfigProvider* config, QObject* parent)
	: QObject(parent)
	, m_config(config)
	, m_bleHandle(nullptr)
{
}

BLEThread::~BLEThread() {
	stop();
	safeUnloadLibrary();
}

// --- IAppModule 接口实现 ---

bool BLEThread::initialize()
{
	// 连接日志到全局单例
	connect(this, &BLEThread::logMessage, &LogManagement::instance(), &LogManagement::logMessage);
	return true;
}

void BLEThread::start()
{
	if (m_workThread) return; // 防止重复启动

	m_workThread = new QThread(this);
	moveToThread(m_workThread);

	connect(m_workThread, &QThread::started, this, &BLEThread::run, Qt::QueuedConnection);

	m_workThread->start();
}

void BLEThread::stop(int timeoutMs)
{
	if (!m_workThread) return;

	if (m_workThread->isRunning())
	{
		m_workThread->quit();
		m_workThread->wait(timeoutMs);
	}

	// 移回当前线程，确保安全析构
	moveToThread(QThread::currentThread());
	m_workThread = nullptr;
}

bool BLEThread::loadBleLibrary()
{
	if (m_bleLoader.isLoaded()) return true;

	if (!m_bleLoader.load("BLEDll.dll", {
		{"CreateBleHandle", (void**)&m_bleFuncs.createHandle},
		{"DestroyBleHandle", (void**)&m_bleFuncs.destroyHandle},
		{"StartBleScanWrapper", (void**)&m_bleFuncs.startScan},
		{"StopBleScanWrapper", (void**)&m_bleFuncs.stopScan},
		{"ConnectBLEDeviceWrapper", (void**)&m_bleFuncs.connectDevice},
		{"DisconnectBLEDeviceWrapper", (void**)&m_bleFuncs.disconnectDevice},
		{"CheckCharacteristicWritePermissionWrapper", (void**)&m_bleFuncs.checkCharacteristicWritePermission},
		{"WriteDataByCharacteristicWrapper", (void**)&m_bleFuncs.writeDataByCharacteristic},
		{"IsConnectedWrapper", (void**)&m_bleFuncs.isConnected}
	}))
	{
		emit logMessage(QString("BLE DLL库%1: %2")
			.arg(m_bleLoader.state() == NativeLibraryLoader::State::LoadFailed ? "加载失败" : "函数解析失败")
			.arg(m_bleLoader.errorString()),
			LogManagement::LOG_ERROR);
		safeUnloadLibrary();
		return false;
	}

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
	// 防重入：如果正在连接/扫描中，拒绝重复重连请求
	if (m_connecting.load(std::memory_order_acquire))
	{
		emit logMessage(QString("BLE正在连接中，请稍后重试"), LogManagement::LogLevel::LOG_WARNING);
		return;
	}
	m_connecting.store(true, std::memory_order_release);

	emit logMessage(QString("BLE重连开始"), LogManagement::LogLevel::LOG_INFO);

	if (m_deviceInfo.isFind) {
		if (m_deviceInfo.isConnected) {
			m_bleFuncs.disconnectDevice(m_bleHandle);
			m_deviceInfo.isConnected = false;
		}
	}

	safeUnloadLibrary();

	// 重置初始化标志，允许重新执行 initializeBle
	m_bleInitialized.store(false, std::memory_order_release);

	if (loadBleLibrary()) {
		initializeBle();
		// initializeBle() 的 performFirstConnection/connectToDevice 末尾设置 m_connecting = false
	}
	else
	{
		emit logMessage(QString("BLE重连失败：DLL加载失败"), LogManagement::LogLevel::LOG_ERROR);
		m_connecting.store(false, std::memory_order_release);
	}
}

void BLEThread::updateConnectionStatus()
{
	// 正在连接中，跳过状态检测（避免干扰连接流程）
	if (m_connecting.load(std::memory_order_acquire)) return;

	bool wasConnected = m_deviceInfo.isConnected;
	ConnectionStatus();
	if (!m_deviceInfo.isConnected && wasConnected) {
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
		sendData(reinterpret_cast<const unsigned char*>(buff), 2);
		emit logMessage(QString("发送数据成功:") + buff, LogManagement::LogLevel::LOG_INFO);
	}
	else
	{
		emit logMessage(QString("BLE连接断开"), LogManagement::LogLevel::LOG_WARNING);
		emit disconnected();
	}
	return;
}

void BLEThread::sendFanMode(FanMode mode)
{
	char buffer[2];
	buffer[0] = '0' + fanModeValue(mode);
	buffer[1] = '\0';
	sendData(reinterpret_cast<const unsigned char*>(buffer), 2);
}

void BLEThread::autoMode()
{
	QMutexLocker lock(&m_modeMutex);

	if (!m_deviceInfo.isFind)return;
	ConnectionStatus();

	if (m_deviceInfo.isConnected)
	{
		sendFanMode(FanMode::Auto);
		emit logMessage(QString("自动模式启动"), LogManagement::LogLevel::LOG_INFO);
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
		sendFanMode(FanMode::Silent);
		emit logMessage(QString("静音模式启动"), LogManagement::LogLevel::LOG_INFO);
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
		sendFanMode(FanMode::Performance);
		emit logMessage(QString("全速模式启动"), LogManagement::LogLevel::LOG_INFO);
	}
	else
	{
		emit logMessage(QString("BLE连接断开"), LogManagement::LogLevel::LOG_WARNING);
		emit disconnected();
	}

	return;
}

void BLEThread::safeUnloadLibrary() {
	if (m_bleHandle && m_bleFuncs.destroyHandle) {
		m_bleFuncs.destroyHandle(m_bleHandle);
		m_bleHandle = nullptr;
	}
	m_bleLoader.unload();
}

void BLEThread::initializeIniFile()
{
	m_config->initSection("BLE", "false");
	m_config->write("BLE", "Init", "false");
	m_config->write("BLE", "TargetName", "BT24-T");
	m_config->write("BLE", "TargetServiceUUID", 0xFFE0);
	m_config->write("BLE", "TargetCharacteristicUUID", 0xFFE1);

	m_config->initSection("BLE", "true");

	emit logMessage(QString("BLE配置文件创建并写入完成"), LogManagement::LogLevel::LOG_INFO);
}

void BLEThread::loadDeviceConfig()
{
	m_deviceInfo.name = m_config->read("BLE", "TargetName").toString();
	m_deviceInfo.serviceUuid = m_config->read("BLE", "TargetServiceUUID").toUInt();
	m_deviceInfo.characteristicUuid = m_config->read("BLE", "TargetCharacteristicUUID").toUInt();
	m_scanCtx.targetName = m_deviceInfo.name;
}

void BLEThread::scanDevicesInternal(ScannedBleDeviceCallback callback, bool setId)
{
	ScannerGuard guard(this);
	m_scanCtx.isFound.store(false, std::memory_order_release);

	emit bleScanStarted();
	QEventLoop loop;
	QTimer timer, timer_maxtime;
	timer.setSingleShot(false);
	timer_maxtime.setSingleShot(true);
	connect(&timer, &QTimer::timeout, [&]() {
		if (m_scanCtx.isFound.load(std::memory_order_acquire))
		{
			if (setId) m_deviceInfo.id = m_scanCtx.foundDeviceId;
			m_deviceInfo.isFind = true;
			timer.stop();
			timer_maxtime.stop();
			loop.quit();
		}
	});
	connect(&timer_maxtime, &QTimer::timeout, [&]() {
		timer.stop();
		timer_maxtime.stop();
		emit bleScanTimeout();
		loop.quit();
	});

	timer.start(1000);
	timer_maxtime.start(10000);
	m_bleFuncs.startScan(m_bleHandle, callback);
	loop.exec();
	m_bleFuncs.stopScan(m_bleHandle);
}

void BLEThread::scanDevices()      { scanDevicesInternal(bleDeviceScannedCallback, true); }
void BLEThread::scanVerifyDevice() { scanDevicesInternal(bleVerifyDeviceCallback, false); }

void BLEThread::initializeBle() {
	// 防止重复初始化
	if (m_bleInitialized.exchange(true, std::memory_order_acquire))
	{
		emit logMessage(QString("BLE已初始化，跳过重复调用"), LogManagement::LogLevel::LOG_DEBUG);
		return;
	}

	{
		QMutexLocker lock(&m_operationMutex);

		m_bleHandle = m_bleFuncs.createHandle();
		if (!m_bleHandle) {
			emit logMessage(QString("BLE句柄创建失败"), LogManagement::LOG_ERROR);
			m_bleInitialized.store(false, std::memory_order_release);
			return;
		}

		if (!m_config->fileExists()) {
			initializeIniFile();
		}

		if (!(m_config->isInitialized("BLE")))
		{
			initializeIniFile();
		}

		loadDeviceConfig();
	}

	// 标记正在连接，防止 updateConnectionStatus 干扰
	m_connecting.store(true, std::memory_order_release);
	if (m_config->read("BLE", "Init").toString() == "false") {
		performFirstConnection();
		return;
	}
	else {
		connectToDevice(m_config->read("BLE", "TargetID").toString());
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
				m_connecting.store(false, std::memory_order_release);
				return;
			}
		}

		emit connectionInProgress();

		if (m_deviceInfo.isFind && m_deviceInfo.id != nullptr) {
			if (m_bleFuncs.connectDevice(m_bleHandle, m_deviceInfo.id.toUtf8().constData())) {
				m_config->write("BLE", "TargetID", m_deviceInfo.id);
				m_config->write("BLE", "Init", "true");
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
	m_connecting.store(false, std::memory_order_release);
}


void BLEThread::connectToDevice(const QString& deviceId)
{
	m_scanCtx.foundDeviceId = deviceId;

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
				m_connecting.store(false, std::memory_order_release);
				return;
			}
		}

		emit connectionInProgress();

		if (m_bleFuncs.connectDevice(m_bleHandle, deviceId.toUtf8().constData())) {
			emit logMessage(QString("连接成功！"), LogManagement::LogLevel::LOG_INFO);
			m_deviceInfo.isConnected = true;
			emit connected();
		}
		else {
			emit logMessage(QString("连接失败！"), LogManagement::LogLevel::LOG_ERROR);
			emit connectionFailed();
		}
	}
	m_connecting.store(false, std::memory_order_release);
}

void BLEThread::ConnectionStatus()
{
	m_deviceInfo.isConnected = m_bleFuncs.isConnected(m_bleHandle);
}

void BLEThread::sendData(const unsigned char* data, size_t length)
{
	if (m_deviceInfo.isConnected) {
		if (m_bleFuncs.writeDataByCharacteristic(m_bleHandle, m_deviceInfo.serviceUuid, m_deviceInfo.characteristicUuid, data, static_cast<unsigned int>(length), true)) {
			emit logMessage(QString("发送成功！"), LogManagement::LogLevel::LOG_INFO);
		} else {
			emit logMessage(QString("BLE写入失败"), LogManagement::LogLevel::LOG_ERROR);
		}
	}
	else
	{
		emit logMessage(QString("数据发送失败！"), LogManagement::LogLevel::LOG_ERROR);
	}
}