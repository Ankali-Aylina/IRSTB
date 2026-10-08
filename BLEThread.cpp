#include "BLEThread.h"
#include <QDebug>
#include <QEventLoop>
#include <cstdio>

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
		self->m_disconnectReported.store(false, std::memory_order_release);
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

void BLEThread::onNotifyReceived(const wchar_t* serviceUuid, const wchar_t* characteristicUuid,
	const uint8_t* data, uint32_t dataLen, void* userData)
{
	// 出口参数未使用（回调签名由 DLL 约定固定，不能省略）
	Q_UNUSED(serviceUuid);
	Q_UNUSED(characteristicUuid);

	auto* self = static_cast<BLEThread*>(userData);
	if (!self || !data || dataLen == 0) return;

	// 回调运行在 DLL 的 BLE 事件线程：只做字节拷贝后交给 Qt 排队处理，
	// 不在此处触碰任何 Qt/成员状态
	self->onDeviceStatusReport(QByteArray(reinterpret_cast<const char*>(data), static_cast<int>(dataLen)));
}

void BLEThread::onDeviceStatusReport(const QByteArray& payload)
{
	// BLE 通知的边界与下位机的发送边界并不对应：一行回传可能被拆成多条通知到货
	// （实测 "OK.180\n" 会分成 "OK" 与 ".180\n"），因此必须先按行切分再解析
	m_notifyBuffer.append(payload);

	// 原始字节记录：出现异常回传时是唯一的定位依据
	emit logMessage(QString::fromUtf8("下位机回传原始数据(%1 字节): %2")
		.arg(payload.size())
		.arg(QString::fromLatin1(payload.toHex(' '))),
		LogManagement::LogLevel::LOG_DEBUG);

	while (true)
	{
		const int idx = m_notifyBuffer.indexOf('\n');

		// 没有换行：可能仍在半行中，等待后续通知
		if (idx < 0)
		{
			// 防御：非法/超长数据不应无限堆积
			if (m_notifyBuffer.size() > 64) m_notifyBuffer.clear();
			return;
		}

		const QByteArray frame = m_notifyBuffer.left(idx);
		m_notifyBuffer.remove(0, idx + 1);

		// 解析一行回传（格式见 tools/README.md）："OK.<秒数>" / "WAIT" / "FIN" / "CAN"
		const QString line = QString::fromLatin1(frame).trimmed();
		if (line.isEmpty()) continue;

		// 收到任何回传都说明链路是通的，允许下一次真正掉线时重新提示
		m_disconnectReported.store(false, std::memory_order_release);

		// OTA 升级进行中：所有回传都交给升级状态机解析
		// （此时串口里流动的是数据帧应答 "A.<序号>" / "E.<序号>"，以及
		//   "OTAOK"/"OTAFAIL" 这类控制应答，与散热事件流无关）
		if (m_otaActive)
		{
			emit otaResponseLine(line);
			continue;
		}

		int remainingSeconds = 0;
		bool finished = false;
		QString text;
		LogManagement::LogLevel level = LogManagement::LOG_INFO;

		if (line.startsWith(QStringLiteral("OK")))
		{
			// "OK.180"：秒数由下位机自报，用于核对上位机设置与实际生效是否一致
			const int dot = line.indexOf(QLatin1Char('.'));
			if (dot > 0) remainingSeconds = line.mid(dot + 1).toInt();
			text = remainingSeconds > 0
				? QString::fromUtf8("下位机确认散热开始：自报 %1 秒").arg(remainingSeconds)
				: QString::fromUtf8("下位机确认散热开始");
			// 中途若断连就收不到 FIN：按自报秒数兜底，避免上位机一直以为还在散热
			if (remainingSeconds > 0) scheduleShutdownFinishedCheck(remainingSeconds);
		}
		else if (line.startsWith(QStringLiteral("TO")))
		{
			// "TO.<已解析位数>.<已过秒数>"：下位机接收超时（1 秒内没等到完整帧）
			text = QString::fromUtf8("下位机接收超时，指令被丢弃（诊断 %1）").arg(line.mid(2));
			level = LogManagement::LOG_WARNING;
			m_shutdownActive.store(false, std::memory_order_release);
			scheduleShutdownRetry();
		}
		else if (line.startsWith(QStringLiteral("WAIT")))
		{
			text = QString::fromUtf8("下位机拒绝了指令（帧格式非法或时长超限），将自动重发一次");
			level = LogManagement::LOG_WARNING;
			// 指令实际未生效：清掉本地标志，避免上位机此后不再下发温度调速帧
			m_shutdownActive.store(false, std::memory_order_release);
			scheduleShutdownRetry();
		}
		else if (line.startsWith(QStringLiteral("FIN")))
		{
			text = QString::fromUtf8("下位机报告：关机散热结束，风扇已断电，正在切回自动模式");
			m_shutdownActive.store(false, std::memory_order_release);
			finished = true;
		}
		else if (line.startsWith(QStringLiteral("CAN")))
		{
			text = QString::fromUtf8("下位机报告：关机散热已被取消");
			m_shutdownActive.store(false, std::memory_order_release);
			finished = true;
		}
		else if (line.startsWith(QLatin1Char(kVersionRespHeader)))
		{
			// "V1.3.0.1.1" = 固件版本 1.3.0 + 协议版本 1.1。
			// 单独处理并直接进入下一行：版本回传不是"指令执行结果"，
			// 不应出现在散热相关的事件流里
			handleVersionResponse(line.mid(1));
			continue;
		}
		else
		{
			text = QString::fromUtf8("下位机回传（未识别）：%1").arg(line);
		}

		emit logMessage(text, level);
		emit deviceStatus(text, remainingSeconds);

		// 散热结束/取消后风扇处于断电状态：先通知界面（可能只是路过提示），
		// 再自动切回自动模式，让风扇按温度继续工作
		if (finished)
		{
			emit shutdownCoolingFinished();
			restoreAutoModeAfterShutdown();
		}
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
		{"BleGetLastWriteResult", (void**)&m_bleFuncs.getLastWriteResult},
		{"BleSubscribeCharacteristic", (void**)&m_bleFuncs.subscribeCharacteristic},
		{"BleUnsubscribeCharacteristic", (void**)&m_bleFuncs.unsubscribeCharacteristic},
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

	// 订阅接口是后加的（DLL 1.0.0 起）：缺失时只提示，不影响风扇控制
	if (!m_bleFuncs.subscribeCharacteristic || !m_bleFuncs.unsubscribeCharacteristic)
	{
		emit logMessage(QString::fromUtf8("该 BLE DLL 不支持特征通知，将无法接收下位机状态回传"),
			LogManagement::LogLevel::LOG_WARNING);
	}

	// 写入结果查询接口（DLL 1.1 起）：缺失时退化为"请求已受理"即视为成功
	if (!m_bleFuncs.getLastWriteResult)
	{
		emit logMessage(QString::fromUtf8("该 BLE DLL 不支持写入结果查询，将无法确认指令是否真的写入"),
			LogManagement::LogLevel::LOG_DEBUG);
	}

	return true;
}

void BLEThread::safeUnloadLibrary()
{
	unsubscribeDeviceStatus();

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
			unsubscribeDeviceStatus();
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
		// 掉线后旧的订阅已失效，复位标志以便重连成功后重新订阅
		m_notifySubscribed.store(false, std::memory_order_release);
		reportDisconnectedOnce();
	}
}

// ============================================================================
// 风扇控制
// ============================================================================

void BLEThread::controlFan(char* buff)
{
	QMutexLocker lock(&m_fanControlMutex);

	// 关机散热倒计时进行中：下位机会忽略普通模式指令，此处不再发送，
	// 避免每秒一次的无效 BLE 写入与误报日志
	if (m_shutdownActive.load(std::memory_order_acquire)) return;

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
		// 温度每 TxDelay 就更新一次，掉线期间会不断走到这里；
		// 去重后只在掉线的那一刻提示一次
		reportDisconnectedOnce();
	}
}

void BLEThread::reportDisconnectedOnce()
{
	// 注意：|| 短路求值确保已上报时不会再次改写标志
	if (m_disconnectReported.load(std::memory_order_acquire)
		|| m_disconnectReported.exchange(true, std::memory_order_acq_rel))
	{
		return;
	}

	emit logMessage(QString::fromUtf8("BLE连接断开"), LogManagement::LogLevel::LOG_WARNING);
	emit disconnected();
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

	if (m_connecting.load(std::memory_order_acquire))
	{
		emit commandNotSent(QString::fromUtf8("蓝牙正在搜索/连接中，指令未下发，请稍后重试"));
		return;
	}
	if (!m_deviceInfo.isFind)
	{
		emit commandNotSent(QString::fromUtf8("尚未找到下位机设备，指令未下发"));
		return;
	}
	ConnectionStatus();

	if (m_deviceInfo.isConnected)
	{
		sendFanMode(FanMode::Auto);
		emit logMessage(QString::fromUtf8("自动模式启动"), LogManagement::LogLevel::LOG_INFO);
	}
	else
	{
		// 之前这里只写日志（用户看不到），界面表现为"点了没反应"
		emit commandNotSent(QString::fromUtf8("蓝牙未连接，指令未下发，请点击“重连”后重试"));
		reportDisconnectedOnce();
	}
}

void BLEThread::silentMode()
{
	QMutexLocker lock(&m_modeMutex);

	if (m_connecting.load(std::memory_order_acquire))
	{
		emit commandNotSent(QString::fromUtf8("蓝牙正在搜索/连接中，指令未下发，请稍后重试"));
		return;
	}
	if (!m_deviceInfo.isFind)
	{
		emit commandNotSent(QString::fromUtf8("尚未找到下位机设备，指令未下发"));
		return;
	}
	ConnectionStatus();

	if (m_deviceInfo.isConnected)
	{
		sendFanMode(FanMode::Silent);
		emit logMessage(QString::fromUtf8("静音模式启动"), LogManagement::LogLevel::LOG_INFO);
	}
	else
	{
		emit commandNotSent(QString::fromUtf8("蓝牙未连接，指令未下发，请点击“重连”后重试"));
		reportDisconnectedOnce();
	}
}

void BLEThread::performanceMode()
{
	QMutexLocker lock(&m_modeMutex);

	if (m_connecting.load(std::memory_order_acquire))
	{
		emit commandNotSent(QString::fromUtf8("蓝牙正在搜索/连接中，指令未下发，请稍后重试"));
		return;
	}
	if (!m_deviceInfo.isFind)
	{
		emit commandNotSent(QString::fromUtf8("尚未找到下位机设备，指令未下发"));
		return;
	}
	ConnectionStatus();

	if (m_deviceInfo.isConnected)
	{
		sendFanMode(FanMode::Performance);
		emit logMessage(QString::fromUtf8("全速模式启动"), LogManagement::LogLevel::LOG_INFO);
	}
	else
	{
		emit commandNotSent(QString::fromUtf8("蓝牙未连接，指令未下发，请点击“重连”后重试"));
		reportDisconnectedOnce();
	}
}

// ============================================================================
// 关机散热
// ============================================================================

bool BLEThread::sendShutdownFrame(int minutes)
{
	// 帧格式 "T<分钟>\n"：ASCII 十进制 + 换行终止符，
	// 下位机按状态机解析，丢字节或非法字符都会被安全丢弃
	char buffer[6];
	int written = snprintf(buffer, sizeof(buffer), "%c%d\n", kShutdownCmdHeader, minutes);
	if (written <= 0 || written >= static_cast<int>(sizeof(buffer)))
	{
		emit logMessage(QString::fromUtf8("关机散热指令组帧失败"), LogManagement::LOG_ERROR);
		return false;
	}

	sendData(reinterpret_cast<const unsigned char*>(buffer), static_cast<size_t>(written),
		LogManagement::LogLevel::LOG_INFO);
	return true;
}

void BLEThread::shutdownCooling(int minutes)
{
	QMutexLocker lock(&m_modeMutex);

	if (m_connecting.load(std::memory_order_acquire))
	{
		emit commandNotSent(QString::fromUtf8("蓝牙正在搜索/连接中，散热指令未下发"));
		return;
	}
	if (!m_deviceInfo.isFind)
	{
		emit commandNotSent(QString::fromUtf8("尚未找到下位机设备，散热指令未下发"));
		return;
	}
	ConnectionStatus();

	if (!m_deviceInfo.isConnected)
	{
		emit logMessage(QString::fromUtf8("关机散热指令下发失败：BLE连接断开"),
			LogManagement::LogLevel::LOG_WARNING);
		emit commandNotSent(QString::fromUtf8("蓝牙未连接，散热指令未下发，请点击“重连”后重试"));
		emit disconnected();
		return;
	}

	// 范围钳制：下位机硬上限为 60 分钟（2 位十进制），越界会被直接丢弃
	int validMinutes = minutes;
	if (validMinutes < kMinShutdownMinutes) validMinutes = kMinShutdownMinutes;
	if (validMinutes > kMaxShutdownMinutes) validMinutes = kMaxShutdownMinutes;
	if (validMinutes != minutes)
	{
		emit logMessage(QString::fromUtf8("关机散热时长 %1 分钟超出范围，已修正为 %2 分钟")
			.arg(minutes).arg(validMinutes), LogManagement::LogLevel::LOG_WARNING);
	}

	// 记录本次请求并重置重试额度（新一轮下发允许再重试一次）
	m_shutdownMinutes.store(validMinutes, std::memory_order_release);
	m_shutdownRetryUsed = false;
	m_autoRestoreDone.store(false, std::memory_order_release);

	if (!sendShutdownFrame(validMinutes))
	{
		return;
	}

	m_shutdownActive.store(true, std::memory_order_release);
	emit logMessage(QString::fromUtf8("已下发关机散热指令：风扇全速运转 %1 分钟后关闭（期间蓝牙断开不影响）")
		.arg(validMinutes), LogManagement::LogLevel::LOG_INFO);
}

void BLEThread::notifyShutdownCancelled()
{
	if (!m_shutdownActive.exchange(false, std::memory_order_acq_rel)) return;

	// 此刻上位机重新接管风扇控制：真正的取消由紧随其后的模式指令（'0'~'6'）完成，
	// 下位机接收到模式指令后即恢复正常模式处理，并会回传 CAN
	emit logMessage(QString::fromUtf8("本地已结束关机散热记录：风扇控制交回上位机"),
		LogManagement::LogLevel::LOG_INFO);
}

// ============================================================================
// 下位机状态通知（特征 Notify 订阅）
// ============================================================================

void BLEThread::subscribeDeviceStatus()
{
	// 旧版 DLL 不导出订阅接口
	if (!m_bleFuncs.subscribeCharacteristic) return;
	if (m_connectionId < 0 || !m_deviceInfo.isConnected) return;

	// 回调可能被重复触发，只订阅一次
	if (m_notifySubscribed.exchange(true, std::memory_order_acq_rel)) return;

	// 丢弃上一次连接遗留的半行数据
	m_notifyBuffer.clear();

	const QString svcUuid = uuidToString(m_deviceInfo.serviceUuid);
	const QString charUuid = uuidToString(m_deviceInfo.characteristicUuid);
	const std::wstring wSvc = svcUuid.toStdWString();
	const std::wstring wChar = charUuid.toStdWString();

	const BleError result = m_bleFuncs.subscribeCharacteristic(
		m_connectionId, wSvc.c_str(), wChar.c_str(), onNotifyReceived, this);

	if (result == BLE_OK)
	{
		emit logMessage(QString::fromUtf8("已订阅下位机状态通知（%1）").arg(charUuid),
			LogManagement::LOG_INFO);

		// 订阅就绪后再查版本。刻意延迟 1.5 秒：实测订阅成功后立刻发指令会被丢弃
		// （连接刚建立时模块/链路尚未稳定），表现为"版本未知"；而连接稳定后
		// 发出的模式指令都能正常送达。旧固件不会回传，届时靠超时判断。
		QTimer::singleShot(1500, this, [this]() {
			if (m_deviceInfo.isConnected && m_connectionId >= 0) {
				queryDeviceVersion();
			}
		});
	}
	else
	{
		// 失败不致命：风扇控制走写入通道，不依赖通知
		m_notifySubscribed.store(false, std::memory_order_release);
		emit logMessage(QString::fromUtf8("订阅下位机状态通知失败：%1（不影响风扇控制）")
			.arg(bleErrorToMessage(result)), LogManagement::LogLevel::LOG_WARNING);
	}
}

void BLEThread::queryDeviceVersion()
{
	// 新连接：先清掉上一次的版本信息，避免显示过期数据
	m_fwVersion.clear();
	m_fwProtocol.clear();
	m_fwSupportsShutdown = false;

	// 版本查询帧 "V\n"：下位机（固件 1.3.0+）回传 "V<固件版本>.<协议版本>"。
	// 用 sendData 走统一通道，发送原始字节与写入结果都会被记录
	const unsigned char frame[2] = { static_cast<unsigned char>(kVersionQueryChar), '\n' };
	sendData(frame, sizeof(frame));

	// 旧固件根本不认识 "V\n"，不会有任何回传：等一会儿若仍无版本信息，
	// 就通知界面"版本未知"，避免界面一直显示"查询中"
	if (!m_versionTimeoutTimer)
	{
		m_versionTimeoutTimer = new QTimer(this);
		m_versionTimeoutTimer->setSingleShot(true);
		connect(m_versionTimeoutTimer, &QTimer::timeout, this, [this]() {
			if (m_fwVersion.isEmpty())
			{
				emit logMessage(QString::fromUtf8(
					"下位机未响应版本查询（固件低于 1.3.0？），固件版本未知"),
					LogManagement::LogLevel::LOG_WARNING);
				// 旧固件（<1.3.0）仍支持关机散热（1.2.0 起），按"支持"处理更保守
				emit firmwareVersionReceived(QString(), QString(), true);
			}
		});
	}
	m_versionTimeoutTimer->start(kVersionQueryTimeoutMs);
}

void BLEThread::handleVersionResponse(const QString& payload)
{
	// 形如 "1.3.0.1.1"：前三段为固件版本，后两段为协议版本
	// （旧固件可能只回 "1.3.0"，此时协议版本视为未知）
	const QStringList parts = payload.split(QLatin1Char('.'), Qt::SkipEmptyParts);
	if (parts.size() < 3)
	{
		emit logMessage(QString::fromUtf8("下位机版本回传格式异常：%1").arg(payload),
			LogManagement::LogLevel::LOG_WARNING);
		return;
	}

	m_fwVersion = QStringLiteral("%1.%2.%3").arg(parts[0], parts[1], parts[2]);
	m_fwProtocol = parts.size() >= 5
		? QStringLiteral("%1.%2").arg(parts[3], parts[4])
		: QString();

	const int major = parts[0].toInt();
	const int minor = parts[1].toInt();

	// 固件是否支持关机散热：低于 1.2.0 会忽略 T 指令（表现为"点了没反应"）
	m_fwSupportsShutdown = (major > kMinFwMajorForShutdown)
		|| (major == kMinFwMajorForShutdown && minor >= kMinFwMinorForShutdown);

	// 收到版本回传：停掉"版本未知"的兜底定时器
	if (m_versionTimeoutTimer) m_versionTimeoutTimer->stop();

	emit logMessage(QString::fromUtf8("下位机固件版本：%1（协议 %2）")
		.arg(m_fwVersion, m_fwProtocol.isEmpty() ? QString::fromUtf8("未知") : m_fwProtocol),
		LogManagement::LOG_INFO);

	if (!m_fwSupportsShutdown)
	{
		emit logMessage(QString::fromUtf8(
			"下位机固件过旧（需 %1.%2.0 及以上），「关机散热」不可用："
			"下位机会忽略该指令，请先升级下位机固件")
			.arg(kMinFwMajorForShutdown).arg(kMinFwMinorForShutdown),
			LogManagement::LogLevel::LOG_WARNING);
	}

	emit firmwareVersionReceived(m_fwVersion, m_fwProtocol, m_fwSupportsShutdown);
}

void BLEThread::unsubscribeDeviceStatus()
{
	if (!m_notifySubscribed.exchange(false, std::memory_order_acq_rel)) return;

	// 断连后残留的半行数据必须丢弃，否则会与下一次连接的字节拼成错误帧
	m_notifyBuffer.clear();

	if (!m_bleFuncs.unsubscribeCharacteristic) return;
	if (m_connectionId < 0) return;
	const QString svcUuid = uuidToString(m_deviceInfo.serviceUuid);
	const QString charUuid = uuidToString(m_deviceInfo.characteristicUuid);
	const std::wstring wSvc = svcUuid.toStdWString();
	const std::wstring wChar = charUuid.toStdWString();

	(void)m_bleFuncs.unsubscribeCharacteristic(m_connectionId, wSvc.c_str(), wChar.c_str());
}

void BLEThread::scheduleShutdownRetry()
{
	// 同一轮下发只重试一次，避免下位机持续拒绝时反复刷指令
	if (m_shutdownRetryUsed) return;
	const int minutes = m_shutdownMinutes.load(std::memory_order_acquire);
	if (minutes <= 0) return;
	m_shutdownRetryUsed = true;

	if (!m_shutdownRetryTimer)
	{
		m_shutdownRetryTimer = new QTimer(this);
		m_shutdownRetryTimer->setSingleShot(true);
		connect(m_shutdownRetryTimer, &QTimer::timeout, this, &BLEThread::retryShutdownCooling);
	}

	emit logMessage(QString::fromUtf8("将在 600ms 后自动重发关机散热指令"), LogManagement::LOG_INFO);
	m_shutdownRetryTimer->start(600);
}

void BLEThread::retryShutdownCooling()
{
	const int minutes = m_shutdownMinutes.load(std::memory_order_acquire);
	if (minutes <= 0) return;

	emit logMessage(QString::fromUtf8("自动重发关机散热指令（%1 分钟）").arg(minutes),
		LogManagement::LOG_INFO);
	shutdownCooling(minutes);
}

void BLEThread::scheduleShutdownFinishedCheck(int seconds)
{
	if (!m_shutdownFinishTimer)
	{
		m_shutdownFinishTimer = new QTimer(this);
		m_shutdownFinishTimer->setSingleShot(true);
		connect(m_shutdownFinishTimer, &QTimer::timeout, this, [this]() {
			// 只有"仍认为在散热"时才兜底：说明中途断连、FIN 回传丢了
			if (!m_shutdownActive.exchange(false, std::memory_order_acq_rel)) return;

			emit logMessage(QString::fromUtf8(
				"按设定时长推断关机散热已结束，但未收到下位机的结束回传（期间可能发生蓝牙断开）"),
				LogManagement::LogLevel::LOG_WARNING);
			emit shutdownCoolingFinished();
		});
	}

	// 多给 5 秒余量，正常情况下 FIN 会先到并把标志清掉，此定时器随即空转
	m_shutdownFinishTimer->start((seconds + 5) * 1000);
}

void BLEThread::restoreAutoModeAfterShutdown()
{
	// FIN 与兜底定时器都可能触发，只恢复一次
	if (m_autoRestoreDone.exchange(true, std::memory_order_acq_rel)) return;

	if (m_connecting.load(std::memory_order_acquire))
	{
		emit commandNotSent(QString::fromUtf8(
			"关机散热已结束，风扇已断电；蓝牙正在重连，请稍后点击风扇模式按钮恢复散热"));
		return;
	}

	ConnectionStatus();
	if (!m_deviceInfo.isConnected)
	{
		emit commandNotSent(QString::fromUtf8(
			"关机散热已结束，风扇已断电；蓝牙未连接，无法自动恢复，请点击“重连”后选择风扇模式"));
		return;
	}

	// 与用户点击"自动"走同一条路径：下位机收尾后不锁定，指令会立即生效
	emit logMessage(QString::fromUtf8("关机散热结束，正在切回自动模式"), LogManagement::LogLevel::LOG_INFO);
	autoMode();
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
	else
	{
		// 连接成功：订阅下位机状态通知（失败只记日志，不影响风扇控制）
		subscribeDeviceStatus();
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

void BLEThread::sendData(const unsigned char* data, size_t length, LogManagement::LogLevel rawLogLevel)
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
		// 连接恢复正常：允许下一次掉线时重新提示
		m_disconnectReported.store(false, std::memory_order_release);
		// 原始字节记录：与"下位机回传原始数据"配对，可完整还原一次握手
		emit logMessage(QString::fromUtf8("发送原始数据(%1 字节): %2")
			.arg(static_cast<int>(length))
			.arg(QString::fromLatin1(QByteArray(reinterpret_cast<const char*>(data),
				static_cast<int>(length)).toHex(' '))),
			rawLogLevel);
		emit logMessage(QString::fromUtf8("发送成功！"), LogManagement::LogLevel::LOG_INFO);

		// 写入是异步的：上面只代表"请求已受理"。若 DLL 支持结果查询，
		// 稍后核对真实结果，避免"其实没写进去却记成功"
		if (m_bleFuncs.getLastWriteResult)
		{
			if (length == 2 && data[0] >= '0' && data[0] <= '6')
				m_pendingWriteWhat = QString::fromUtf8("模式指令 %1").arg(QChar(data[0]));
			else
				m_pendingWriteWhat = QString::fromUtf8("数据帧");
			m_pendingWriteIsShutdown = (length > 2 && data[0] == kShutdownCmdHeader);

			if (!m_writeConfirmTimer)
			{
				m_writeConfirmTimer = new QTimer(this);
				m_writeConfirmTimer->setSingleShot(true);
				connect(m_writeConfirmTimer, &QTimer::timeout, this, &BLEThread::confirmLastWrite);
			}
			m_writeConfirmTimer->start(400);
		}
	}
	else
	{
		emit logMessage(QString::fromUtf8("BLE写入失败: %1").arg(bleErrorToMessage(result)),
			LogManagement::LogLevel::LOG_ERROR);
	}
}

// ============================================================================
// OTA 固件升级
//
// 流程（与下位机 boot/boot_main.c 的接收循环对应）：
//   1. 发 "OTA\n"  → 应用置位元数据并复位 → 引导程序接管串口 → 回 "OTAOK"
//   2. 逐帧发送 0xA5 数据帧，每帧等 "A.<序号>" 应答；超时/收到 "E.<序号>" 则重发
//   3. 全部发完发 "OTAE\n" → 引导程序做整镜像 CRC32 复核 → 回 "OTAOK"（成功）
//      或 "OTAFAIL"（失败）
//
// 为什么必须逐帧等应答：串口是 9600 baud，蓝牙与串口之间还有模块的缓冲，
// 无应答地连发会丢帧，而丢帧意味着烧进去的镜像 CRC 必然不匹配。
// ============================================================================

/// <summary>CRC16-CCITT（多项式 0x1021，初值 0xFFFF）——与下位机 Crc16_Update 一致</summary>
static uint16_t otaCrc16(const uint8_t* data, int len)
{
	uint16_t crc = 0xFFFFu;

	for (int i = 0; i < len; ++i)
	{
		crc ^= static_cast<uint16_t>(data[i]) << 8;
		for (int b = 0; b < 8; ++b)
		{
			crc = (crc & 0x8000u) ? static_cast<uint16_t>((crc << 1) ^ 0x1021u)
			                      : static_cast<uint16_t>(crc << 1);
		}
	}
	return crc;
}

void BLEThread::otaResetState()
{
	if (m_otaAckTimer) m_otaAckTimer->stop();
	if (m_otaBootPollTimer) m_otaBootPollTimer->stop();
	m_otaImage.clear();
	m_otaSent = 0;
	m_otaSeq = 0;
	m_otaRetry = 0;
	m_otaActive = false;
	m_otaWaitingAck = false;
	m_otaFinishing = false;
	m_otaPhase = OtaPhase::Idle;
	m_otaChunk = kOtaChunkMax;
	m_otaLastTake = 0;
	m_otaBootPollCount = 0;
}

bool BLEThread::loadFirmwareImage(const QString& filePath, QByteArray& image, QString& error)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly))
	{
		error = QString::fromUtf8("无法打开固件文件：%1").arg(file.errorString());
		return false;
	}
	const QByteArray raw = file.readAll();
	file.close();

	if (raw.isEmpty())
	{
		error = QString::fromUtf8("固件文件为空");
		return false;
	}

	// .bin：直接就是镜像
	if (!filePath.endsWith(QStringLiteral(".hex"), Qt::CaseInsensitive))
	{
		image = raw;
		return true;
	}

	// .hex：解析 Intel HEX，取地址连续的最大段（即应用镜像本体）
	// 记录格式：:LLAAAATT[DD...]CC
	QMap<quint32, QByteArray> segs;   // 起始地址 → 数据
	quint32 upper = 0;                // 扩展线性地址
	bool sawEof = false;

	const QList<QByteArray> lines = raw.split('\n');
	for (const QByteArray& rawLine : lines)
	{
		const QByteArray line = rawLine.trimmed();
		if (line.isEmpty() || line[0] != ':') continue;

		bool ok = false;
		(void)ok;
		const QByteArray body = QByteArray::fromHex(line.mid(1));
		if (body.size() < 5) { error = QString::fromUtf8("HEX 记录过短"); return false; }

		const quint8 len = static_cast<quint8>(body[0]);
		const quint16 addr = static_cast<quint16>((static_cast<quint8>(body[1]) << 8) |
		                                          static_cast<quint8>(body[2]));
		const quint8 type = static_cast<quint8>(body[3]);
		if (body.size() < 5 + len) { error = QString::fromUtf8("HEX 记录长度不符"); return false; }

		if (type == 0x00)                       // 数据
		{
			segs[upper + addr] = body.mid(4, len);
		}
		else if (type == 0x04)                  // 扩展线性地址
		{
			upper = (static_cast<quint32>(static_cast<quint8>(body[4])) << 24) |
			        (static_cast<quint32>(static_cast<quint8>(body[5])) << 16);
		}
		else if (type == 0x01)                  // 文件结束
		{
			sawEof = true;
			break;
		}
	}

	if (segs.isEmpty())
	{
		error = QString::fromUtf8("HEX 文件里没有数据记录");
		return false;
	}
	(void)sawEof;

	// 只保留从最低地址开始连续的那一段：应用镜像在 Flash 中是连续的，
	// 而 HEX 里可能还夹带别的段（如配置区），拼接起来会破坏镜像
	quint32 start = segs.firstKey();
	QByteArray out;
	quint32 expect = start;
	for (auto it = segs.constBegin(); it != segs.constEnd(); ++it)
	{
		if (it.key() != expect) break;          // 出现空洞：到此为止
		out += it.value();
		expect = it.key() + static_cast<quint32>(it.value().size());
	}

	if (out.isEmpty())
	{
		error = QString::fromUtf8("HEX 解析结果为空");
		return false;
	}
	image = out;
	return true;
}

void BLEThread::startFirmwareUpgrade(const QString& filePath)
{
	if (m_otaActive)
	{
		emit otaFailed(QString::fromUtf8("已有升级正在进行"));
		return;
	}
	if (m_connectionId < 0 || !m_deviceInfo.isConnected)
	{
		emit otaFailed(QString::fromUtf8("蓝牙未连接，无法升级"));
		return;
	}

	QByteArray image;
	QString error;
	if (!loadFirmwareImage(filePath, image, error))
	{
		emit otaFailed(error);
		return;
	}
	// 槽位容量 20KB（见下位机 memory_map.h 的 SLOT_SIZE）
	if (image.size() > 20 * 1024)
	{
		emit otaFailed(QString::fromUtf8("固件过大：%1 字节，超出下位机 20KB 槽位")
			.arg(image.size()));
		return;
	}

	otaResetState();
	m_otaImage = image;
	m_otaActive = true;

	// 升级期间不要再下发温度调速/散热指令，避免与数据帧抢串口
	m_shutdownActive.store(true, std::memory_order_release);

	emit logMessage(QString::fromUtf8("开始固件升级：%1（%2 字节）")
		.arg(QFileInfo(filePath).fileName()).arg(m_otaImage.size()),
		LogManagement::LOG_INFO);
	emit otaStateChanged(QString::fromUtf8("正在请求下位机进入升级模式…"));
	emit otaProgress(0, m_otaImage.size());

	if (!m_otaAckTimer)
	{
		m_otaAckTimer = new QTimer(this);
		m_otaAckTimer->setSingleShot(true);
		connect(m_otaAckTimer, &QTimer::timeout, this, [this]() {
			if (!m_otaActive) return;
			// 帧应答超时：重发当前帧
			if (m_otaWaitingAck)
			{
				if (++m_otaRetry > kOtaMaxRetry)
				{
					otaAbort(QString::fromUtf8("下位机连续 %1 次未应答，升级中止")
						.arg(kOtaMaxRetry));
					return;
				}
				emit logMessage(QString::fromUtf8("第 %1 帧应答超时，重发（第 %2 次）")
					.arg(m_otaSeq).arg(m_otaRetry), LogManagement::LOG_WARNING);
				m_otaWaitingAck = false;
				otaSendNextChunk();
				return;
			}
			// 非数据帧阶段的超时：按阶段给出能直接定位的原因
			switch (m_otaPhase)
			{
			case OtaPhase::EnterApp:
				otaAbort(QString::fromUtf8(
					"下位机未响应进入升级模式的请求（设备上运行的固件可能不是 OTA 版本）"));
				break;
			case OtaPhase::EnterBoot:
				otaAbort(QString::fromUtf8(
					"引导程序未响应升级请求（设备可能没有烧录引导程序）"));
				break;
			case OtaPhase::Finishing:
				otaAbort(QString::fromUtf8(
					"下位机校验超时（镜像可能超出 20KB 槽位，或写 Flash 失败）"));
				break;
			default:
				otaAbort(QString::fromUtf8("下位机未进入升级模式（未收到应答）"));
				break;
			}
		});
	}

	// 复位等待期的轮询：引导程序进 OTA 模式时**不会主动上报任何东西**
	// （见下位机 BootMain：拿到 SRAM 请求标志后直接进 RunOtaSession 静默等待），
	// 所以必须靠 V\n 的回传来判断它是否已接管：应用回 "V1.x"，引导程序回 "V0.x"
	if (!m_otaBootPollTimer)
	{
		m_otaBootPollTimer = new QTimer(this);
		m_otaBootPollTimer->setInterval(kOtaBootPollIntervalMs);
		connect(m_otaBootPollTimer, &QTimer::timeout, this, [this]() {
			if (!m_otaActive || m_otaPhase != OtaPhase::WaitBoot) return;

			if (m_connectionId < 0 || !m_deviceInfo.isConnected)
			{
				otaAbort(QString::fromUtf8("升级过程中蓝牙连接断开"));
				return;
			}
			if (++m_otaBootPollCount > kOtaBootPollMax)
			{
				otaAbort(QString::fromUtf8("下位机未在 %1 秒内进入升级模式（复位后无响应）")
					.arg(kOtaBootPollMax * kOtaBootPollIntervalMs / 1000));
				return;
			}

			// 只查询版本、不重发 OTA\n：应用还活着时重发会让它再复位一次，
			// 等于把等待窗口无限往后推
			const unsigned char frame[2] = { static_cast<unsigned char>(kVersionQueryChar), '\n' };
			sendData(frame, sizeof(frame));
		});
	}

	// 把状态机接到应答行上（只接一次）。m_otaWired 是**实例成员**：
	// 用函数内 static 的话，模块重建（新建 BLEThread）后再也不会连接
	if (!m_otaWired)
	{
		connect(this, &BLEThread::otaResponseLine, this, &BLEThread::handleOtaResponse);
		m_otaWired = true;
	}

	// 第一步：请**应用**进入升级模式。应用回 "OTAOK" 后置 SRAM 请求标志并软复位，
	// 随后由引导程序接管 —— 注意引导程序不会主动报"我进来了"，见上面的轮询
	const QByteArray cmd(kOtaEnterCmd);
	sendData(reinterpret_cast<const unsigned char*>(cmd.constData()),
		static_cast<size_t>(cmd.size()));

	m_otaPhase = OtaPhase::EnterApp;
	m_otaAckTimer->start(kOtaAckTimeoutMs);   // 应用应答复很快；复位等待另走轮询
}

void BLEThread::otaSendNextChunk()
{
	if (!m_otaActive || m_otaWaitingAck) return;

	const qint64 remain = m_otaImage.size() - m_otaSent;
	if (remain <= 0)
	{
		otaRequestFinish();
		return;
	}

	const int take = static_cast<int>(qMin<qint64>(remain, m_otaChunk));
	QByteArray frame;
	frame.reserve(kOtaFrameOverhead + take);
	frame.append(static_cast<char>(kOtaFrameSync));
	frame.append(static_cast<char>(m_otaSeq));
	frame.append(static_cast<char>(take & 0xFF));
	frame.append(static_cast<char>((take >> 8) & 0xFF));
	frame.append(m_otaImage.constData() + m_otaSent, take);

	// CRC16 覆盖"序号 + 长度 + 负载"，与下位机一致
	const uint16_t crc = otaCrc16(reinterpret_cast<const uint8_t*>(frame.constData()) + 1,
		frame.size() - 1);
	frame.append(static_cast<char>(crc & 0xFF));
	frame.append(static_cast<char>((crc >> 8) & 0xFF));

	sendData(reinterpret_cast<const unsigned char*>(frame.constData()),
		static_cast<size_t>(frame.size()));

	m_otaLastTake = take;
	m_otaWaitingAck = true;
	m_otaAckTimer->start(kOtaAckTimeoutMs);
}

void BLEThread::handleOtaResponse(const QString& line)
{
	if (!m_otaActive) return;

	// ---- 等复位完成 ----
	// 引导程序进 OTA 模式时**不主动上报**，只能靠它回 "V0.x" 判断已经接管。
	// 这一步是关键：应用回 OTAOK 时引导程序还没起来，此时若开始传数据，
	// 每一帧都会被拒收（引导程序 s_session_on 仍为 0）→ E.xx → 重试耗尽 → 中止
	if (m_otaPhase == OtaPhase::WaitBoot)
	{
		if (line.startsWith(QLatin1String(kOtaBootVersionPrefix)))
		{
			m_otaBootPollTimer->stop();
			emit logMessage(QString::fromUtf8("下位机已进入引导程序（%1），请求建立升级会话")
				.arg(line), LogManagement::LOG_INFO);
			emit otaStateChanged(QString::fromUtf8("已进入升级模式，正在建立会话…"));

			const QByteArray cmd(kOtaEnterCmd);
			sendData(reinterpret_cast<const unsigned char*>(cmd.constData()),
				static_cast<size_t>(cmd.size()));
			m_otaPhase = OtaPhase::EnterBoot;
			m_otaAckTimer->start(kOtaAckTimeoutMs);
			return;
		}
		// 应用还在跑（回 "V1.x"）或复位期间无响应：继续轮询，不做别的
		return;
	}

	// "OTAOK"：进入升级模式应答，或最终激活成功的应答
	if (line.startsWith(QLatin1String(kOtaRespOk)))
	{
		if (m_otaPhase == OtaPhase::EnterApp)
		{
			// **应用**已受理，它随即置 SRAM 标志并软复位。这里绝不能开始传数据：
			// 引导程序尚未接管，数据帧会被全部拒收。改为等复位完成
			m_otaAckTimer->stop();
			m_otaPhase = OtaPhase::WaitBoot;
			m_otaBootPollCount = 0;
			emit otaStateChanged(QString::fromUtf8("下位机正在重启进入升级模式…"));
			emit logMessage(QString::fromUtf8("应用已受理升级请求，等待其复位后由引导程序接管"),
				LogManagement::LOG_INFO);
			m_otaBootPollTimer->start();
			return;
		}

		if (m_otaPhase == OtaPhase::EnterBoot)
		{
			// **引导程序**的会话已建立，现在才可以下发数据帧
			m_otaAckTimer->stop();
			m_otaPhase = OtaPhase::Streaming;
			emit otaStateChanged(QString::fromUtf8("会话已建立，正在传输固件…"));
			emit logMessage(QString::fromUtf8("引导程序已就绪，开始传输固件"),
				LogManagement::LOG_INFO);
			otaSendNextChunk();
			return;
		}

		if (m_otaFinishing)
		{
			// 引导程序已完成整镜像校验并写元数据，即将复位激活
			const qint64 total = m_otaImage.size();
			otaResetState();
			m_shutdownActive.store(false, std::memory_order_release);
			emit otaProgress(total, total);
			emit otaStateChanged(QString::fromUtf8("校验通过，下位机正在激活新固件…"));
			emit logMessage(QString::fromUtf8("固件升级成功，下位机将复位并运行新固件"),
				LogManagement::LOG_INFO);
			emit otaFinished();

			// 下位机马上会复位跑新固件。**必须主动重查版本**：BLE 连接由透传模块
			// 保持，MCU 复位不会断链，因此不会有"连接成功→查询版本"来刷新它，
			// 否则界面会一直显示升级前的旧版本号
			QTimer::singleShot(kOtaVersionRequeryDelayMs, this, [this]() {
				requeryVersionAfterOta(0);
			});
			return;
		}

		return;   // 其他阶段的 OTAOK 无意义，忽略
	}

	if (line.startsWith(QLatin1String(kOtaRespFail)))
	{
		otaAbort(m_otaFinishing
			? QString::fromUtf8("下位机校验失败（镜像可能不完整，请重试）")
			: QString::fromUtf8("下位机拒绝进入升级模式"));
		return;
	}

	// 数据帧应答："A.<序号>" / "E.<序号>"（只在传输阶段有意义）
	if (m_otaPhase != OtaPhase::Streaming) return;

	if (line.size() >= 2)
	{
		const QChar kind = line.at(0);
		const int ackSeq = line.mid(2).toInt(nullptr, 16);

		if (kind == QLatin1Char(kOtaAckOk))
		{
			if (!m_otaWaitingAck) return;       // 重复应答，忽略
			m_otaAckTimer->stop();
			m_otaWaitingAck = false;
			m_otaRetry = 0;
			// 按**实际发出**的长度推进：帧长可能已被下调过，不能用常量算
			m_otaSent += m_otaLastTake;
			m_otaSeq++;
			emit otaProgress(m_otaSent, m_otaImage.size());
			otaSendNextChunk();
			return;
		}
		if (kind == QLatin1Char(kOtaAckFail))
		{
			// 下位机校验/写失败，要求重发该帧
			if (++m_otaRetry > kOtaMaxRetry)
			{
				otaAbort(QString::fromUtf8("第 %1 帧反复写入失败，升级中止").arg(ackSeq));
				return;
			}
			// 同一帧连续被拒，多半不是数据本身错，而是**帧太长**：超过 BLE MTU 时
			// 下位机收到的是残帧，CRC 自然不过。把负载减半再试。
			// 下位机是"按顺序追加写"，帧长中途变化不影响镜像正确性
			if (m_otaRetry >= 2 && m_otaChunk > kOtaChunkMin)
			{
				m_otaChunk = qMax(kOtaChunkMin, m_otaChunk / 2);
				m_otaRetry = 0;
				emit logMessage(QString::fromUtf8(
					"数据帧连续被拒，负载长度下调为 %1 字节（蓝牙 MTU 可能小于帧长）")
					.arg(m_otaChunk), LogManagement::LOG_WARNING);
			}
			m_otaAckTimer->stop();
			m_otaWaitingAck = false;
			otaSendNextChunk();
			return;
		}
	}

	// 其他回传（如散热状态）在升级期间忽略
}

void BLEThread::otaRequestFinish()
{
	if (m_otaFinishing) return;
	m_otaFinishing = true;
	m_otaPhase = OtaPhase::Finishing;
	emit otaStateChanged(QString::fromUtf8("传输完成，等待下位机校验…"));

	const QByteArray cmd(kOtaEndCmd);
	sendData(reinterpret_cast<const unsigned char*>(cmd.constData()),
		static_cast<size_t>(cmd.size()));
	m_otaAckTimer->start(kOtaAckTimeoutMs * 10);   // 整镜像 CRC 校验需要时间
}

void BLEThread::otaAbort(const QString& reason)
{
	otaResetState();
	m_shutdownActive.store(false, std::memory_order_release);
	emit logMessage(QString::fromUtf8("固件升级失败：%1").arg(reason),
		LogManagement::LOG_ERROR);
	emit otaFailed(reason);
}

void BLEThread::cancelFirmwareUpgrade()
{
	if (!m_otaActive) return;

	// 尽量告知下位机放弃（引导程序收到后会跳回原应用）
	const QByteArray cmd(kOtaQuitCmd);
	sendData(reinterpret_cast<const unsigned char*>(cmd.constData()),
		static_cast<size_t>(cmd.size()));

	// ⚠️ 在"等复位"阶段取消时，应用已经把 SRAM 请求标志置上了：OTAQ 现在发出去
	// 没人接（引导程序还没起来），设备下一拍仍会进引导程序并停在升级模式。
	// 这不影响设备安全（引导程序不会自己刷写任何东西），但用户需要知道
	// "下次连上会看到引导程序，可再次升级或再发一次取消"
	const bool beforeBoot = (m_otaPhase == OtaPhase::WaitBoot);

	otaResetState();
	m_shutdownActive.store(false, std::memory_order_release);
	emit otaStateChanged(QString::fromUtf8("已取消升级"));
	emit logMessage(beforeBoot
		? QString::fromUtf8("用户取消了固件升级（应用已受理，设备可能仍会进入升级模式；"
			"下次连上若是引导程序，可重新升级或再次取消）")
		: QString::fromUtf8("用户取消了固件升级"),
		LogManagement::LOG_WARNING);
	emit otaFailed(QString::fromUtf8("已取消"));
}

void BLEThread::requeryVersionAfterOta(int attempt)
{
	if (!m_deviceInfo.isConnected || m_connectionId < 0)
	{
		return;
	}

	// queryDeviceVersion() 会先清空 m_fwVersion，拿不到就是空的（界面显示"版本未知"）
	queryDeviceVersion();

	if (attempt >= kOtaVersionRequeryRetries)
	{
		return;
	}

	// 等这次查询的超时窗口过去：仍是空说明这次没问到，再试一次
	QTimer::singleShot(kVersionQueryTimeoutMs + 500, this, [this, attempt]() {
		if (m_fwVersion.isEmpty())
		{
			emit logMessage(QString::fromUtf8("升级后首次查询固件版本未响应，重试一次"),
				LogManagement::LogLevel::LOG_WARNING);
			requeryVersionAfterOta(attempt + 1);
		}
	});
}

void BLEThread::confirmLastWrite()
{
	// 旧版 DLL 没有这个接口，或没有待确认的写入
	if (!m_bleFuncs.getLastWriteResult || m_pendingWriteWhat.isEmpty()) return;

	const QString what = m_pendingWriteWhat;
	const bool wasShutdown = m_pendingWriteIsShutdown;
	m_pendingWriteWhat.clear();
	m_pendingWriteIsShutdown = false;

	// 连接可能在这 400ms 内断掉
	if (m_connectionId < 0 || !m_deviceInfo.isConnected)
	{
		emit logMessage(QString::fromUtf8("%1 未能确认：连接已断开").arg(what),
			LogManagement::LogLevel::LOG_WARNING);
		if (wasShutdown) scheduleShutdownRetry();
		return;
	}

	const QString charUuid = uuidToString(m_deviceInfo.characteristicUuid);
	const std::wstring wChar = charUuid.toStdWString();
	const BleError res = m_bleFuncs.getLastWriteResult(m_connectionId, wChar.c_str());

	if (res == BLE_OK)
	{
		emit logMessage(QString::fromUtf8("%1 写入已确认成功").arg(what),
			LogManagement::LogLevel::LOG_DEBUG);
		return;
	}

	// 仍是 PENDING：异步写入还没出结果，属于正常情况，不当作失败
	if (res == BLE_WRITE_PENDING)
	{
		emit logMessage(QString::fromUtf8("%1 写入结果尚未返回（异步进行中）").arg(what),
			LogManagement::LogLevel::LOG_DEBUG);
		return;
	}

	emit logMessage(QString::fromUtf8("%1 写入失败：%2").arg(what, bleErrorToMessage(res)),
		LogManagement::LogLevel::LOG_ERROR);

	// 关机散热帧没真的写进去：效果等同于下位机没收到，走同一条自动重发逻辑
	if (wasShutdown) scheduleShutdownRetry();
}
