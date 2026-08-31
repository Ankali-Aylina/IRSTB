#include "BLEThread.h"
#include <QDebug>
#include <QEventLoop>

// ============================================================================
// 静态回调 — 通过 userData 路由到 BLEThread 实例
// ============================================================================

void BLEThread::onDeviceScanned(const wchar_t* address, const wchar_t* name, int16_t rssi, void* userData)
{
	auto* self = static_cast<BLEThread*>(userData);
	if (!self) return;

	qDebug() << "Device scanned:" << QString::fromWCharArray(name ? name : L"")
	         << QString::fromWCharArray(address ? address : L"") << rssi;

	auto& ctx = self->m_scanCtx;

	// 回调可能来自 DLL 内部线程，QString 访问必须加锁
	QMutexLocker lock(&ctx.mutex);

	// 按名称匹配（首次扫描）
	if (!ctx.targetName.isEmpty() && name && ctx.targetName == QString::fromWCharArray(name))
	{
		ctx.foundAddress = QString::fromWCharArray(address ? address : L"");
		ctx.isFound.store(true, std::memory_order_release);
		return;
	}

	// 按地址匹配（验证扫描）
	if (!ctx.foundAddress.isEmpty() && address && ctx.foundAddress == QString::fromWCharArray(address))
	{
		ctx.isFound.store(true, std::memory_order_release);
		return;
	}
}

void BLEThread::onFirstConnResult(const wchar_t* address, int connected, const wchar_t* error, void* userData)
{
	// address 未使用（回调签名由 DLL 约定固定，不能省略）
	Q_UNUSED(address);

	auto* self = static_cast<BLEThread*>(userData);
	if (!self) return;

	// 回调可能被 DLL 重复触发（连接成功事件可能到达两次），
	// 成功结果只接受一次，避免重复发出 connected() 信号
	if (connected == 1 && self->m_connResult.load(std::memory_order_acquire))
		return;

	self->m_connCallbackFired.store(true, std::memory_order_release);
	self->m_connResult.store(connected == 1, std::memory_order_release);
	self->m_deviceInfo.isConnected.store(connected == 1, std::memory_order_release);

	if (connected)
	{
		self->emit logMessage(QString::fromUtf8("连接成功！"), LogManagement::LOG_INFO);
		self->emit connected();
	}
	else
	{
		QString errMsg = error ? QString::fromWCharArray(error) : QString::fromUtf8("未知错误");
		self->emit logMessage(QString::fromUtf8("连接失败！") + errMsg, LogManagement::LOG_ERROR);
		self->emit connectionFailed();
	}
}

QString BLEThread::uuidToString(quint16 uuid)
{
	return QString::asprintf("%04X", uuid);
}

QString BLEThread::bleErrorToMessage(BleError err) const
{
	const wchar_t* errStr = m_bleFuncs.errorToString
		? m_bleFuncs.errorToString(err) : L"Unknown error";
	QString msg = QString::fromWCharArray(errStr);

	// 针对常见错误码附加排查建议
	switch (err) {
	case BLE_ERROR_DEVICE_NOT_FOUND:
		msg += QString::fromUtf8("；请确认设备已开机且在范围内");
		break;
	case BLE_ERROR_ACCESS_DENIED:
		msg += QString::fromUtf8("；请在 Windows 蓝牙设置中先配对设备");
		break;
	case BLE_ERROR_UNREACHABLE:
		msg += QString::fromUtf8("；设备可能已连接至其他主机或超出范围");
		break;
	case BLE_ERROR_NOT_CONNECTED:
		msg += QString::fromUtf8("；请先建立蓝牙连接");
		break;
	case BLE_ERROR_SERVICE_NOT_FOUND:
		msg += QString::fromUtf8("；请检查 ServiceUUID 配置");
		break;
	case BLE_ERROR_CHAR_NOT_FOUND:
		msg += QString::fromUtf8("；请检查 CharacteristicUUID 配置");
		break;
	case BLE_ERROR_WRITE_FAILED:
		msg += QString::fromUtf8("；请确认特征值支持写入操作");
		break;
	case BLE_ERROR_GATT_FAILED:
		msg += QString::fromUtf8("；GATT 通信异常，请尝试重新连接");
		break;
	default:
		break;
	}

	return msg;
}

// ============================================================================
// 构造 / 析构
// ============================================================================

BLEThread::BLEThread(IConfigProvider* config, QObject* parent)
	: QObject(parent)
	, m_config(config)
{
}

BLEThread::~BLEThread() {
	stop();
	safeUnloadLibrary();
}

// ============================================================================
// IAppModule 接口
// ============================================================================

bool BLEThread::initialize()
{
	connect(this, &BLEThread::logMessage, &LogManagement::instance(), &LogManagement::logMessage);
	return true;
}

void BLEThread::start()
{
	if (m_workThread) return;

	// 支持 stop() 后重新 start()：复位停止标志
	m_stopping.store(false, std::memory_order_release);

	m_workThread = new QThread(this);
	moveToThread(m_workThread);

	connect(m_workThread, &QThread::started, this, &BLEThread::run, Qt::QueuedConnection);

	m_workThread->start();
}

void BLEThread::stop(int timeoutMs)
{
	if (!m_workThread) return;

	// 通知所有嵌套事件循环（扫描/连接等待）立即退出
	m_stopping.store(true, std::memory_order_release);

	if (m_workThread->isRunning())
	{
		m_workThread->quit();
		if (!m_workThread->wait(timeoutMs))
		{
			// 嵌套循环响应 m_stopping 后会在极短时间内退出，这里必须等到线程真正结束，
			// 否则 QThread 会在仍运行时被销毁（崩溃/卡死）
			m_workThread->wait();
		}
	}

	moveToThread(QThread::currentThread());
	m_workThread = nullptr;
}

// ============================================================================
// DLL 加载 / 卸载
// ============================================================================

bool BLEThread::loadBleLibrary()
{
	if (m_bleLoader.isLoaded()) return true;

	if (!m_bleLoader.load("WinRT_BLE_DLL.dll", {
		{"BleInitialize", (void**)&m_bleFuncs.initialize},
		{"BleUninitialize", (void**)&m_bleFuncs.uninitialize},
		{"BleStartScan", (void**)&m_bleFuncs.startScan},
		{"BleStopScan", (void**)&m_bleFuncs.stopScan},
		{"BleConnect", (void**)&m_bleFuncs.connect},
		{"BleDisconnect", (void**)&m_bleFuncs.disconnect},
		{"BleIsConnected", (void**)&m_bleFuncs.isConnected},
		{"BleWriteCharacteristic", (void**)&m_bleFuncs.writeCharacteristic},
		{"BleGetLastError", (void**)&m_bleFuncs.getLastError},
		{"BleErrorToString", (void**)&m_bleFuncs.errorToString}
	}))
	{
		emit logMessage(QString::fromUtf8("BLE DLL库%1: %2")
			.arg(m_bleLoader.state() == NativeLibraryLoader::State::LoadFailed
				? QString::fromUtf8("加载失败") : QString::fromUtf8("函数解析失败"))
			.arg(m_bleLoader.errorString()),
			LogManagement::LOG_ERROR);
		safeUnloadLibrary();
		return false;
	}

	return true;
}

void BLEThread::safeUnloadLibrary()
{
	if (m_connectionId >= 0 && m_bleFuncs.disconnect)
	{
		m_bleFuncs.disconnect(m_connectionId);
		m_connectionId = -1;
	}
	if (m_bleFuncs.uninitialize)
	{
		m_bleFuncs.uninitialize();
	}
	m_bleLoader.unload();
	m_deviceInfo.isConnected = false;
}

// ============================================================================
// 入口
// ============================================================================

void BLEThread::run()
{
	if (m_stopping.load(std::memory_order_acquire))
		return;

	if (loadBleLibrary())
	{
		initializeBle();
	}
	else
	{
		// DLL 加载失败时也必须通知 GUI，否则界面永远停留在"初始化…"且按钮无任何反应
		emit connectionFailed();
	}
}

// ============================================================================
// 重连
// ============================================================================

void BLEThread::reconnectDevice()
{
	if (m_connecting.load(std::memory_order_acquire))
	{
		emit logMessage(QString::fromUtf8("BLE正在连接中，请稍后重试"), LogManagement::LogLevel::LOG_WARNING);
		return;
	}
	m_connecting.store(true, std::memory_order_release);

	emit logMessage(QString::fromUtf8("BLE重连开始"), LogManagement::LogLevel::LOG_INFO);

	if (m_deviceInfo.isFind)
	{
		if (m_deviceInfo.isConnected && m_connectionId >= 0)
		{
			m_bleFuncs.disconnect(m_connectionId);
			m_connectionId = -1;
			m_deviceInfo.isConnected = false;
		}
	}

	safeUnloadLibrary();
	m_bleInitialized.store(false, std::memory_order_release);

	if (loadBleLibrary())
	{
		initializeBle();
	}
	else
	{
		emit logMessage(QString::fromUtf8("BLE重连失败：DLL加载失败"), LogManagement::LogLevel::LOG_ERROR);
		m_connecting.store(false, std::memory_order_release);
		emit connectionFailed();
	}
}

// ============================================================================
// 连接状态检测
// ============================================================================

void BLEThread::updateConnectionStatus()
{
	if (m_connecting.load(std::memory_order_acquire)) return;

	bool wasConnected = m_deviceInfo.isConnected;
	ConnectionStatus();
	if (!m_deviceInfo.isConnected && wasConnected)
	{
		emit disconnected();
	}
}

// ============================================================================
// 风扇控制
// ============================================================================

void BLEThread::controlFan(char* buff)
{
	QMutexLocker lock(&m_fanControlMutex);

	// 扫描/连接过程中不处理风扇数据：此时 ConnectionStatus() 会误判为断开，
	// 反复发出"BLE连接断开"与 disconnected()，导致 GUI 状态乱跳
	if (m_connecting.load(std::memory_order_acquire)) return;
	if (!m_deviceInfo.isFind) return;
	ConnectionStatus();

	if (m_deviceInfo.isConnected)
	{
		sendData(reinterpret_cast<const unsigned char*>(buff), 2);
		emit logMessage(QString::fromUtf8("发送数据成功:") + buff, LogManagement::LogLevel::LOG_INFO);
	}
	else
	{
		emit logMessage(QString::fromUtf8("BLE连接断开"), LogManagement::LogLevel::LOG_WARNING);
		emit disconnected();
	}
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

	if (m_connecting.load(std::memory_order_acquire)) return;
	if (!m_deviceInfo.isFind) return;
	ConnectionStatus();

	if (m_deviceInfo.isConnected)
	{
		sendFanMode(FanMode::Auto);
		emit logMessage(QString::fromUtf8("自动模式启动"), LogManagement::LogLevel::LOG_INFO);
	}
	else
	{
		emit logMessage(QString::fromUtf8("BLE连接断开"), LogManagement::LogLevel::LOG_WARNING);
		emit disconnected();
	}
}

void BLEThread::silentMode()
{
	QMutexLocker lock(&m_modeMutex);

	if (m_connecting.load(std::memory_order_acquire)) return;
	if (!m_deviceInfo.isFind) return;
	ConnectionStatus();

	if (m_deviceInfo.isConnected)
	{
		sendFanMode(FanMode::Silent);
		emit logMessage(QString::fromUtf8("静音模式启动"), LogManagement::LogLevel::LOG_INFO);
	}
	else
	{
		emit logMessage(QString::fromUtf8("BLE连接断开"), LogManagement::LogLevel::LOG_WARNING);
		emit disconnected();
	}
}

void BLEThread::performanceMode()
{
	QMutexLocker lock(&m_modeMutex);

	if (m_connecting.load(std::memory_order_acquire)) return;
	if (!m_deviceInfo.isFind) return;
	ConnectionStatus();

	if (m_deviceInfo.isConnected)
	{
		sendFanMode(FanMode::Performance);
		emit logMessage(QString::fromUtf8("全速模式启动"), LogManagement::LogLevel::LOG_INFO);
	}
	else
	{
		emit logMessage(QString::fromUtf8("BLE连接断开"), LogManagement::LogLevel::LOG_WARNING);
		emit disconnected();
	}
}

// ============================================================================
// 配置
// ============================================================================

void BLEThread::initializeIniFile()
{
	m_config->initSection("BLE", "false");
	m_config->write("BLE", "Init", "false");
	m_config->write("BLE", "TargetName", "BT24-T");
	m_config->write("BLE", "TargetServiceUUID", 0xFFE0);
	m_config->write("BLE", "TargetCharacteristicUUID", 0xFFE1);

	m_config->initSection("BLE", "true");

	emit logMessage(QString::fromUtf8("BLE配置文件创建并写入完成"), LogManagement::LogLevel::LOG_INFO);
}

void BLEThread::loadDeviceConfig()
{
	m_deviceInfo.name = m_config->read("BLE", "TargetName").toString();
	m_deviceInfo.serviceUuid = static_cast<quint16>(m_config->read("BLE", "TargetServiceUUID").toUInt());
	m_deviceInfo.characteristicUuid = static_cast<quint16>(m_config->read("BLE", "TargetCharacteristicUUID").toUInt());
	{
		QMutexLocker lock(&m_scanCtx.mutex);
		m_scanCtx.targetName = m_deviceInfo.name;
	}
}

// ============================================================================
// 扫描
// ============================================================================

void BLEThread::scanDevicesInternal(bool matchByName, bool setId, bool emitStarted)
{
	m_scanCtx.isFound.store(false, std::memory_order_release);

	// 设置匹配目标
	if (matchByName)
	{
		// 按名称匹配：使用已加载的 targetName，清空地址匹配
		QMutexLocker lock(&m_scanCtx.mutex);
		m_scanCtx.targetName = m_deviceInfo.name;
	}
	// else: 按地址匹配（foundAddress 已在 connectToDevice 中设置，targetName 被清空）

	if (emitStarted)
		emit bleScanStarted();

	m_scanId = m_bleFuncs.startScan(onDeviceScanned, this);
	if (m_scanId < 0)
	{
		BleError err = m_bleFuncs.getLastError ? m_bleFuncs.getLastError() : BLE_ERROR_INTERNAL;
		emit logMessage(QString::fromUtf8("启动扫描失败: %1").arg(bleErrorToMessage(err)),
			LogManagement::LOG_ERROR);
		if (emitStarted)
			emit bleScanTimeout();
		return;
	}

	QEventLoop loop;
	QTimer timer, timer_maxtime;
	timer.setSingleShot(false);
	timer_maxtime.setSingleShot(true);

	connect(&timer, &QTimer::timeout, [&]() {
		if (m_stopping.load(std::memory_order_acquire))
		{
			timer.stop();
			timer_maxtime.stop();
			loop.quit();
			return;
		}
		if (m_scanCtx.isFound.load(std::memory_order_acquire))
		{
			if (setId && matchByName)
			{
				QMutexLocker lock(&m_scanCtx.mutex);
				m_deviceInfo.address = m_scanCtx.foundAddress;
			}
			m_deviceInfo.isFind.store(true, std::memory_order_release);
			timer.stop();
			timer_maxtime.stop();
			loop.quit();
		}
	});

	connect(&timer_maxtime, &QTimer::timeout, [&]() {
		timer.stop();
		timer_maxtime.stop();
		if (emitStarted)
			emit bleScanTimeout();
		loop.quit();
	});

	timer.start(1000);
	timer_maxtime.start(10000);
	loop.exec();

	m_bleFuncs.stopScan(m_scanId);
	m_scanId = -1;
}

// 扫描统一由 scanDevicesInternal 完成；重试以循环形式集成在 performFirstConnection/connectToDevice 中

// ============================================================================
// 初始化和连接
// ============================================================================

void BLEThread::initializeBle()
{
	if (m_stopping.load(std::memory_order_acquire))
		return;

	if (m_bleInitialized.exchange(true, std::memory_order_acquire))
	{
		emit logMessage(QString::fromUtf8("BLE已初始化，跳过重复调用"), LogManagement::LogLevel::LOG_DEBUG);
		return;
	}

	int initResult = m_bleFuncs.initialize();
	if (initResult != 0)
	{
		emit logMessage(QString::fromUtf8("BLE初始化失败: %1 (错误码: %2)")
			.arg(bleErrorToMessage(initResult)).arg(initResult),
			LogManagement::LOG_ERROR);
		m_bleInitialized.store(false, std::memory_order_release);
		// 必须复位连接标志并通知 GUI，否则 m_connecting 永久卡死，
		// 重连按钮与状态检测从此全部失效（GUI 无反应）
		m_connecting.store(false, std::memory_order_release);
		emit connectionFailed();
		return;
	}

	{
		QMutexLocker lock(&m_operationMutex);

		if (!m_config->fileExists())
		{
			initializeIniFile();
		}

		if (!(m_config->isInitialized("BLE")))
		{
			initializeIniFile();
		}

		loadDeviceConfig();
	}

	if (m_stopping.load(std::memory_order_acquire))
		return;

	m_connecting.store(true, std::memory_order_release);
	m_deviceInfo.retryCount = 0;

	if (m_config->read("BLE", "Init").toString() == "false")
	{
		performFirstConnection();
	}
	else
	{
		connectToDevice(m_config->read("BLE", "TargetID").toString());
	}
}

// ============================================================================
// 异步连接 + 等待
// ============================================================================

bool BLEThread::connectAndWait(const QString& address)
{
	m_connResult.store(false, std::memory_order_release);
	m_connCallbackFired.store(false, std::memory_order_release);

	std::wstring wAddr = address.toStdWString();
	m_connectionId = m_bleFuncs.connect(wAddr.c_str(), onFirstConnResult, this);

	if (m_connectionId < 0)
	{
		BleError err = m_bleFuncs.getLastError ? m_bleFuncs.getLastError() : BLE_ERROR_INTERNAL;
		emit logMessage(QString::fromUtf8("发起连接失败: %1").arg(bleErrorToMessage(err)), LogManagement::LOG_ERROR);
		emit connectionFailed();
		return false;
	}

	emit connectionInProgress();

	// 等待连接回调（最多 15 秒）
	QEventLoop loop;
	QTimer timeoutTimer;
	timeoutTimer.setSingleShot(true);

	// 定期检查 isConnected 作为兜底
	QTimer pollTimer;
	pollTimer.setSingleShot(false);

	connect(&pollTimer, &QTimer::timeout, [&]() {
		if (m_stopping.load(std::memory_order_acquire))
		{
			loop.quit();
			return;
		}
		if (m_bleFuncs.isConnected && m_bleFuncs.isConnected(m_connectionId) == 1)
		{
			m_connResult.store(true, std::memory_order_release);
			m_deviceInfo.isConnected.store(true, std::memory_order_release);
			loop.quit();
		}
	});

	connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

	timeoutTimer.start(15000);
	pollTimer.start(500);
	loop.exec();

	pollTimer.stop();
	timeoutTimer.stop();

	bool result = m_connResult.load(std::memory_order_acquire);

	if (!result)
	{
		// 超时/失败：立即取消仍挂起的连接操作。若放任不管，DLL 内部会残留
		// 异步连接，之后 unload/reload 会破坏其状态，导致"概率性一直连不上"
		if (m_connectionId >= 0 && m_bleFuncs.disconnect)
		{
			m_bleFuncs.disconnect(m_connectionId);
		}
		m_connectionId = -1;
		m_deviceInfo.isConnected.store(false, std::memory_order_release);

		// 仅当回调从未报告结果（纯超时）时补发失败信号，避免 GUI 卡在"连接中…"
		if (!m_connCallbackFired.load(std::memory_order_acquire))
		{
			emit logMessage(QString::fromUtf8("连接超时，已取消本次连接"), LogManagement::LogLevel::LOG_WARNING);
			emit connectionFailed();
		}
	}

	return result;
}

// ============================================================================
// 首次连接（按名称扫描设备）
// ============================================================================

void BLEThread::performFirstConnection()
{
	// 首次连接：按名称扫描（最多 3 次），找到后连接。
	// 注意：重试必须用循环而非递归——旧实现中递归发生在持有 m_operationMutex 时，
	// 重试函数内部再次加锁（非递归 QMutex）导致死锁，BLE 线程永久停摆，GUI 无反应。
	while (m_deviceInfo.retryCount < 3)
	{
		scanDevicesInternal(true, true, m_deviceInfo.retryCount == 0);

		if (m_stopping.load(std::memory_order_acquire))
			break;

		if (m_deviceInfo.isFind.load(std::memory_order_acquire)
			&& !m_deviceInfo.address.isEmpty())
		{
			if (connectAndWait(m_deviceInfo.address))
			{
				m_config->write("BLE", "TargetID", m_deviceInfo.address);
				m_config->write("BLE", "Init", "true");
				m_deviceInfo.isConnected.store(true, std::memory_order_release);
			}
			break;
		}

		emit logMessage(QString::fromUtf8("扫描超时，未找到设备！"), LogManagement::LogLevel::LOG_WARNING);
		m_deviceInfo.retryCount++;
	}

	if (m_deviceInfo.retryCount >= 3)
	{
		emit logMessage(QString::fromUtf8("连接失败！已超过最大重连次数！"), LogManagement::LogLevel::LOG_ERROR);
		emit bleScanTimeout();
	}

	m_connecting.store(false, std::memory_order_release);
}

// ============================================================================
// 后续连接（按保存的地址直接连接）
// ============================================================================

void BLEThread::connectToDevice(const QString& address)
{
	{
		QMutexLocker lock(&m_scanCtx.mutex);
		m_scanCtx.foundAddress = address;
	}

	// 按地址验证扫描（最多 3 次），找到后连接。同样使用循环避免重试死锁
	while (m_deviceInfo.retryCount < 3)
	{
		scanDevicesInternal(false, false, m_deviceInfo.retryCount == 0);

		if (m_stopping.load(std::memory_order_acquire))
			break;

		if (m_deviceInfo.isFind.load(std::memory_order_acquire))
		{
			if (connectAndWait(address))
			{
				m_deviceInfo.isConnected.store(true, std::memory_order_release);
			}
			break;
		}

		emit logMessage(QString::fromUtf8("设备不在范围内！"), LogManagement::LogLevel::LOG_WARNING);
		m_deviceInfo.retryCount++;
	}

	if (m_deviceInfo.retryCount >= 3)
	{
		emit logMessage(QString::fromUtf8("连接失败！已超过最大重连次数！"), LogManagement::LogLevel::LOG_ERROR);
		emit bleScanTimeout();
	}

	m_connecting.store(false, std::memory_order_release);
}

// ============================================================================
// 状态检查与数据发送
// ============================================================================

void BLEThread::ConnectionStatus()
{
	if (m_connectionId >= 0 && m_bleFuncs.isConnected)
	{
		m_deviceInfo.isConnected = (m_bleFuncs.isConnected(m_connectionId) == 1);
	}
	else
	{
		m_deviceInfo.isConnected = false;
	}
}

void BLEThread::sendData(const unsigned char* data, size_t length)
{
	if (!m_deviceInfo.isConnected || m_connectionId < 0)
	{
		emit logMessage(QString::fromUtf8("数据发送失败！设备未连接"), LogManagement::LogLevel::LOG_ERROR);
		return;
	}

	QString svcUuid = uuidToString(m_deviceInfo.serviceUuid);
	QString charUuid = uuidToString(m_deviceInfo.characteristicUuid);
	std::wstring wSvc = svcUuid.toStdWString();
	std::wstring wChar = charUuid.toStdWString();

	BleError result = m_bleFuncs.writeCharacteristic(
		m_connectionId,
		wSvc.c_str(),
		wChar.c_str(),
		data,
		static_cast<uint32_t>(length)
	);

	if (result == BLE_OK)
	{
		emit logMessage(QString::fromUtf8("发送成功！"), LogManagement::LogLevel::LOG_INFO);
	}
	else
	{
		emit logMessage(QString::fromUtf8("BLE写入失败: %1").arg(bleErrorToMessage(result)),
			LogManagement::LogLevel::LOG_ERROR);
	}
}
