// BLEThread.h
#pragma once

#include <QObject>
#include <QMutex>
#include <QThread>
#include <QTimer>
#include <QByteArray>

#include <atomic>

#include "LogManagement.h"
#include "IniManagement.h"
#include "IAppModule.h"
#include "IConfigProvider.h"
#include "NativeLibraryLoader.h"
#include "TemperatureConfig.h"

// WinRT_BLE_DLL 错误码（对应 API_REFERENCE.md §2）
using BleError = int;
constexpr BleError BLE_OK                       =   0;  // 成功
constexpr BleError BLE_ERROR_NOT_INITIALIZED    =  -1;  // BleInitialize() 未调用
constexpr BleError BLE_ERROR_INVALID_PARAM      =  -2;  // 空指针或空字符串
constexpr BleError BLE_ERROR_DEVICE_NOT_FOUND   =  -3;  // 未找到设备，需先在 Windows 设置中配对
constexpr BleError BLE_ERROR_ACCESS_DENIED      =  -4;  // 访问被拒，需在 Windows 蓝牙设置中配对
constexpr BleError BLE_ERROR_UNREACHABLE        =  -5;  // 超出范围或已连接至其他主机
constexpr BleError BLE_ERROR_GATT_FAILED        =  -6;  // GATT 通信错误
constexpr BleError BLE_ERROR_NOT_CONNECTED      =  -7;  // 未连接任何设备
constexpr BleError BLE_ERROR_SERVICE_NOT_FOUND  =  -8;  // 设备上未找到该 Service UUID
constexpr BleError BLE_ERROR_CHAR_NOT_FOUND     =  -9;  // 服务中未找到该 Characteristic UUID
constexpr BleError BLE_ERROR_READ_FAILED        = -10;  // 读取操作失败
constexpr BleError BLE_ERROR_WRITE_FAILED       = -11;  // 写入操作失败
constexpr BleError BLE_ERROR_NOTIFY_UNSUPPORTED = -12;  // 特征不支持 Notify/Indicate
constexpr BleError BLE_ERROR_INTERNAL           = -99;  // 内部错误

// WinRT_BLE_DLL 回调类型
using BleScanCallback = void(*)(const wchar_t* address, const wchar_t* name, int16_t rssi, void* userData);
using BleConnectionCallback = void(*)(const wchar_t* address, int connected, const wchar_t* error, void* userData);
/// <summary>特征通知回调（下位机主动上报数据时触发，运行在 DLL 的 BLE 事件线程）</summary>
using BleNotifyCallback = void(*)(const wchar_t* serviceUuid, const wchar_t* characteristicUuid,
	const uint8_t* data, uint32_t dataLen, void* userData);

// ============================================================================
// 下位机 BLE 指令协议
// ============================================================================

/// <summary>风扇模式指令：单个 ASCII 字符 '0'~'6'（下位机 FanPWM_SetMode）
/// 关机散热指令帧头：帧格式 "T&lt;分钟&gt;\n"，如 "T3\n" 表示全速散热 3 分钟后停风扇</summary>
constexpr char kShutdownCmdHeader = 'T';

/// <summary>关机散热时长下限（分钟），与下位机一致</summary>
constexpr int kMinShutdownMinutes = 1;

/// <summary>关机散热时长上限（分钟）。下位机硬上限为 60（2 位十进制），
/// 如需超过 10 分钟只需改此常量，无需重新烧录下位机</summary>
constexpr int kMaxShutdownMinutes = 10;

/// <summary>关机散热时长默认值（分钟）</summary>
constexpr int kDefaultShutdownMinutes = 3;

/// <summary>版本查询指令帧：发送 "V\n"，下位机回传 "V&lt;固件版本&gt;.&lt;协议版本&gt;\n"</summary>
constexpr char kVersionQueryChar = 'V';

/// <summary>版本回传帧的帧头</summary>
constexpr char kVersionRespHeader = 'V';

/// <summary>支持「关机散热」所需的最低固件版本。
/// 低于此版本时下位机会忽略 T 指令（表现为"点开始散热没反应"），
/// 因此上位机要主动提示用户升级固件。
/// ⚠️ 这是**经验阈值**而非协议规定：它记录"哪个固件版本起实现了该功能"。
///    固件新增功能并推进版本号时，必须回来同步复核这两个值。</summary>
constexpr int kMinFwMajorForShutdown = 1;
constexpr int kMinFwMinorForShutdown = 2;

/// <summary>支持「版本查询」所需的最低固件版本（低于此版本不会有回传）。
/// 实测固件 1.2.0 已能正常回传 "V1.2.0.1.1"，故阈值由 1.3 下调为 1.2</summary>
constexpr int kMinFwMajorForVersionQuery = 1;
constexpr int kMinFwMinorForVersionQuery = 2;

/// <summary>版本查询的等待时长：超时仍未收到回传即认为固件不支持查询</summary>
constexpr int kVersionQueryTimeoutMs = 2500;

// ============================================================================
// OTA 固件升级协议（与下位机 boot/ota.h、boot_main.c 对应）
// ============================================================================

/// <summary>进入 OTA 模式：应用收到后置位元数据并复位，由引导程序接管串口</summary>
constexpr char kOtaEnterCmd[] = "OTA\n";

/// <summary>结束传输：引导程序做整镜像 CRC32 复核，通过则激活新镜像</summary>
constexpr char kOtaEndCmd[] = "OTAE\n";

/// <summary>放弃本次升级</summary>
constexpr char kOtaQuitCmd[] = "OTAQ\n";

/// <summary>数据帧同步字节</summary>
constexpr uint8_t kOtaFrameSync = 0xA5;

/// <summary>单帧最大负载（与下位机 OTA_CHUNK_MAX 一致）</summary>
constexpr int kOtaChunkMax = 128;

/// <summary>数据帧固定开销：同步(1) + 序号(1) + 长度(2) + CRC16(2)</summary>
constexpr int kOtaFrameOverhead = 6;

/// <summary>应答前缀：'A'=已写入、'E'=失败需重发</summary>
constexpr char kOtaAckOk = 'A';
constexpr char kOtaAckFail = 'E';

/// <summary>进入 OTA 模式的应答（引导程序就绪）</summary>
constexpr char kOtaRespOk[] = "OTAOK";

/// <summary>失败应答前缀</summary>
constexpr char kOtaRespFail[] = "OTAFAIL";

/// <summary>等待应答的超时（毫秒）。9600 baud 下单帧约 140ms，留足余量</summary>
constexpr int kOtaAckTimeoutMs = 1500;

/// <summary>单帧最大重发次数</summary>
constexpr int kOtaMaxRetry = 5;

/// <summary>数据帧负载下限。帧太长导致反复被拒时逐级下调（见 m_otaChunk）</summary>
constexpr int kOtaChunkMin = 20;

/// <summary>引导程序版本回传的帧头，用于判断"复位是否已经完成"。
/// 下位机两个程序的版本号天然可区分：引导程序回 "V0.1.0"（OTA_BOOT_VERSION_STR），
/// 应用回 "V1.x.y"（FW_VERSION_STR）。
/// ⚠️ 若将来引导程序版本升到 1.x，这个判据会失效，必须同步修改
///    （更稳的做法是让引导程序进 OTA 模式时主动报一次，但那要改下位机）。</summary>
constexpr char kOtaBootVersionPrefix[] = "V0.";

/// <summary>轮询"引导程序是否就绪"的间隔（毫秒）</summary>
constexpr int kOtaBootPollIntervalMs = 500;

/// <summary>轮询次数上限（约 500ms × 24 = 12 秒）。复位 + 引导程序启动通常 &lt;1 秒</summary>
constexpr int kOtaBootPollMax = 24;

/// <summary>升级成功后重新查询下位机版本的延时（毫秒）。
/// 下位机收到 OTAE 的确认后会复位并运行新固件，留出它启动的时间再查。
/// ⚠️ 必须重查：BLE 连接由**透传模块**保持，MCU 复位并不会断链，因此不会触发
///    "连接成功→查询版本"那条路径，界面会一直显示升级前的旧版本号
///    （否则刷完固件看到的还是老版本，看起来像"没升上去"）。</summary>
constexpr int kOtaVersionRequeryDelayMs = 2500;

/// <summary>升级后重查版本的最大尝试次数（首次 + 重试一次）。
/// 第一次可能正好撞上下位机启动过程而丢掉，重试一次更稳</summary>
constexpr int kOtaVersionRequeryRetries = 1;

// WinRT_BLE_DLL 函数指针类型
using BleInitializeFunc = int(*)();
using BleUninitializeFunc = void(*)();
using BleStartScanFunc = int(*)(BleScanCallback, void*);
using BleStopScanFunc = void(*)(int);
using BleConnectFunc = int(*)(const wchar_t*, BleConnectionCallback, void*);
using BleDisconnectFunc = void(*)(int);
using BleIsConnectedFunc = int(*)(int);
using BleWriteCharacteristicFunc = BleError(*)(int, const wchar_t*, const wchar_t*, const uint8_t*, uint32_t);
/// <summary>查询某特征上一次写入的真实结果（DLL 1.1+）；BLE_WRITE_PENDING 表示尚无结果</summary>
using BleGetLastWriteResultFunc = BleError(*)(int, const wchar_t*);
using BleSubscribeCharacteristicFunc = BleError(*)(int, const wchar_t*, const wchar_t*, BleNotifyCallback, void*);
using BleUnsubscribeCharacteristicFunc = BleError(*)(int, const wchar_t*, const wchar_t*);
using BleGetLastErrorFunc = BleError(*)();
using BleErrorToStringFunc = const wchar_t*(*)(BleError);

/// <summary>DLL 的"写入尚无结果"状态码：既不是成功也不是失败</summary>
constexpr BleError BLE_WRITE_PENDING = -100;

class BLEThread : public QObject, public IAppModule {
	Q_OBJECT

public:
	explicit BLEThread(IConfigProvider* config, QObject* parent = nullptr);
	~BLEThread();

	// --- IAppModule 接口 ---
	bool initialize() override;
	void start() override;
	void stop(int timeoutMs = 3000) override;

public slots:
	void run();

	/// <summary>设备重连</summary>
	void reconnectDevice();

	/// <summary>检测蓝牙连接状态</summary>
	void updateConnectionStatus();

	/// <summary>风扇转速控制</summary>
	void controlFan(char* buff);

	/// <summary>关机散热：全速运行 minutes 分钟后关闭 PWM 与风扇电源。
	/// 倒计时由下位机 MCU 独立完成，期间蓝牙断开也不中断</summary>
	/// <param name="minutes">散热时长（分钟），超出 [kMinShutdownMinutes, kMaxShutdownMinutes] 会被钳制</param>
	void shutdownCooling(int minutes);

	/// <summary>通知用户已接管风扇控制（关机散热倒计时被上位机指令中止）</summary>
	void notifyShutdownCancelled();

	/// <summary>接收下位机通过特征通知上报的状态行（OK.<秒> / TO.<诊断> / WAIT / FIN / CAN）</summary>
	void onDeviceStatusReport(const QByteArray& payload);

	/// <summary>关机散热指令被拒后的自动重试（BLE 帧偶发丢失时用）</summary>
	void retryShutdownCooling();

	/// <summary>散热结束后切回自动模式（温度驱动调速）。
	/// 下位机在收尾时只关风扇、不锁定，因此该指令会被立即执行</summary>
	void restoreAutoModeAfterShutdown();

	/// <summary>自动模式</summary>
	void autoMode();

	/// <summary>静音模式</summary>
	void silentMode();

	/// <summary>全速模式</summary>
	void performanceMode();

signals:
	void bleScanStarted();
	void bleScanTimeout();
	void connectionInProgress();
	void connectionFailed();
	void connected();
	void disconnected();
	/// <summary>下位机自报状态（已格式化为中文），供界面/日志显示。
	/// 第二个参数为倒计时剩余秒数，仅在收到 OK 时有效</summary>
	void deviceStatus(const QString& text, int remainingSeconds);

	/// <summary>关机散热已结束（风扇已被下位机断电），可以恢复普通模式了</summary>
	void shutdownCoolingFinished();

	/// <summary>指令没能下发出去（未连接/正在连接）：需要提示用户，否则界面看起来"点了没反应"</summary>
	void commandNotSent(const QString& reason);

	/// <summary>收到下位机版本回传。firmware 为固件版本（如 "1.3.0"），
	/// protocol 为协议版本（如 "1.1"）；supportsShutdownCooling 表示固件是否够新</summary>
	void firmwareVersionReceived(const QString& firmware, const QString& protocol, bool supportsShutdownCooling);

	// ---- OTA 固件升级 ----
	/// <summary>升级进度：sent 已确认字节数、total 总字节数</summary>
	void otaProgress(qint64 sent, qint64 total);
	/// <summary>升级阶段性状态（进入升级模式、正在传输、校验中…），供界面显示</summary>
	void otaStateChanged(const QString& text);
	/// <summary>升级成功（新镜像已激活）</summary>
	void otaFinished();
	/// <summary>升级失败，reason 为中文原因</summary>
	void otaFailed(const QString& reason);
	void logMessage(const QString& message, LogManagement::LogLevel level);

private:
	/// <summary>BLE DLL 函数指针表</summary>
	struct BLEFunctionTable {
		BleInitializeFunc initialize = nullptr;
		BleUninitializeFunc uninitialize = nullptr;
		BleStartScanFunc startScan = nullptr;
		BleStopScanFunc stopScan = nullptr;
		BleConnectFunc connect = nullptr;
		BleDisconnectFunc disconnect = nullptr;
		BleIsConnectedFunc isConnected = nullptr;
		BleWriteCharacteristicFunc writeCharacteristic = nullptr;
		BleGetLastWriteResultFunc getLastWriteResult = nullptr;
		BleSubscribeCharacteristicFunc subscribeCharacteristic = nullptr;
		BleUnsubscribeCharacteristicFunc unsubscribeCharacteristic = nullptr;
		BleGetLastErrorFunc getLastError = nullptr;
		BleErrorToStringFunc errorToString = nullptr;
	};

	/// <summary>扫描上下文</summary>
	struct ScanContext {
		// DLL 回调线程与 BLE 工作线程会并发读写以下 QString（隐式共享非线程安全），
		// 必须用互斥锁保护；isFound 保持原子，配合 release/acquire 建立 happens-before
		QMutex mutex;
		QString targetName;
		QString foundAddress;
		std::atomic<bool> isFound{ false };
	};

	struct BleDeviceInfo {
		QString address;       // 设备蓝牙地址（十进制字符串）
		QString name;          // 设备名称
		quint16 serviceUuid = 0xFFE0;       // 服务UUID（16位）
		quint16 characteristicUuid = 0xFFE1; // 特征UUID（16位）
		int retryCount = 0;
		// DLL 回调线程与工作线程会并发读写，必须使用原子类型
		std::atomic<bool> isFind{ false };
		std::atomic<bool> isConnected{ false };
	};

	// 回调（静态方法，通过 userData 路由到实例）
	static void onDeviceScanned(const wchar_t* address, const wchar_t* name, int16_t rssi, void* userData);

	// 内部连接等待回调
	static void onFirstConnResult(const wchar_t* address, int connected, const wchar_t* error, void* userData);

	/// <summary>特征通知回调（静态方法，通过 userData 路由到实例）</summary>
	static void onNotifyReceived(const wchar_t* serviceUuid, const wchar_t* characteristicUuid,
		const uint8_t* data, uint32_t dataLen, void* userData);

	int m_scanId = -1;
	int m_connectionId = -1;
	NativeLibraryLoader m_bleLoader;
	BLEFunctionTable m_bleFuncs;
	ScanContext m_scanCtx;
	BleDeviceInfo m_deviceInfo;
	QMutex m_operationMutex;
	QMutex m_fanControlMutex;
	QMutex m_modeMutex;
	IConfigProvider* m_config;
	QThread* m_workThread = nullptr;

	// 并发控制
	std::atomic<bool> m_bleInitialized{ false };
	std::atomic<bool> m_connecting{ false };
	std::atomic<bool> m_connResult{ false }; // 连接回调结果
	std::atomic<bool> m_connCallbackFired{ false }; // 连接回调是否已到达（区分超时与回调失败）
	std::atomic<bool> m_stopping{ false };   // 停止标志：嵌套事件循环须响应它以便快速退出
	// 关机散热倒计时进行中：期间不再下发温度调速帧，避免无意义的 BLE 写入与日志
	std::atomic<bool> m_shutdownActive{ false };
	// 最近一次请求的散热时长（分钟），用于指令被拒后自动重发
	std::atomic<int> m_shutdownMinutes{ 0 };
	// 掉线提示是否已上报：连接断开后温度更新仍会周期性触发，用于抑制重复的"BLE连接断开"日志
	std::atomic<bool> m_disconnectReported{ false };
	// 是否已订阅下位机状态通知（避免重复订阅；断连/卸载时复位）
	std::atomic<bool> m_notifySubscribed{ false };
	// 通知帧缓冲：BLE 通知不保证"一行一条"，必须按 \n 切分后再解析（否则 OK 与 .180 会被拆成两条）
	QByteArray m_notifyBuffer;
	// 关机散热指令被下位机拒绝后的自动重试（最多一次）
	bool m_shutdownRetryUsed = false;
	QTimer* m_shutdownRetryTimer = nullptr;
	// 关风扇后是否已自动切回自动模式（避免 FIN 与兜底定时器都触发时重复下发）
	std::atomic<bool> m_autoRestoreDone{ false };
	// 按自报秒数兜底判定"散热已结束"：BLE 中途断连时收不到 FIN
	QTimer* m_shutdownFinishTimer = nullptr;
	// 待确认的写入：写入是异步的，"请求已受理"不等于"真的写成功"
	QString m_pendingWriteWhat;
	bool m_pendingWriteIsShutdown = false;
	QTimer* m_writeConfirmTimer = nullptr;
	// 下位机版本（收到回传后填充；为空表示未知/旧固件不支持查询）
	QString m_fwVersion;
	QString m_fwProtocol;
	bool m_fwSupportsShutdown = false;
	QTimer* m_versionTimeoutTimer = nullptr;

	/// <summary>将 16 位 UUID 整数转为宽字符串（如 0xFFE0 → "FFE0"）</summary>
	static QString uuidToString(quint16 uuid);

	/// <summary>将 BleError 转为带排查建议的日志消息</summary>
	QString bleErrorToMessage(BleError err) const;

	void initializeIniFile();
	void loadDeviceConfig();

	void scanDevicesInternal(bool matchByName, bool setId, bool emitStarted);

	void initializeBle();

	void performFirstConnection();
	void connectToDevice(const QString& address);

	/// <summary>异步连接并等待回调结果</summary>
	bool connectAndWait(const QString& address);

	void ConnectionStatus();
	void sendData(const unsigned char* buffer, size_t length,
		LogManagement::LogLevel rawLogLevel = LogManagement::LOG_DEBUG);
	void sendFanMode(FanMode mode);

	/// <summary>上报一次"连接断开"（日志 + disconnected 信号）；同一轮掉线只上报一次，
	/// 避免温度更新周期性地重复刷同一条日志</summary>
	void reportDisconnectedOnce();

	/// <summary>发送 "T&lt;分钟&gt;\n" 关机散热指令帧（调用前须确认连接状态）</summary>
	bool sendShutdownFrame(int minutes);

	/// <summary>订阅下位机状态通知（连接成功后调用；失败只记日志，不影响风扇控制）</summary>
	void subscribeDeviceStatus();

	/// <summary>查询下位机固件/协议版本（连接并订阅成功后调用）</summary>
	void queryDeviceVersion();

	/// <summary>解析版本回传帧 "V&lt;固件版本&gt;.&lt;协议版本&gt;"（不含帧头与换行）</summary>
	void handleVersionResponse(const QString& payload);

	// ---- OTA 固件升级 ----
public slots:
	/// <summary>开始升级：filePath 为 .bin 或 Intel HEX 固件文件</summary>
	void startFirmwareUpgrade(const QString& filePath);
	/// <summary>取消正在进行的升级</summary>
	void cancelFirmwareUpgrade();

signals:
	/// <summary>内部使用：向升级状态机投递一条已解析的下位机应答行</summary>
	void otaResponseLine(const QString& line);

private:
	/// <summary>把固件文件读成镜像字节数组（.bin 直读；.hex 解析 Intel HEX 并取连续段）</summary>
	static bool loadFirmwareImage(const QString& filePath, QByteArray& image, QString& error);

	/// <summary>发送下一个数据帧；无更多数据时转入结束流程</summary>
	void otaSendNextChunk();

	/// <summary>处理升级过程中的下位机应答行</summary>
	void handleOtaResponse(const QString& line);

	/// <summary>以失败结束升级（清理状态并发信号）</summary>
	void otaAbort(const QString& reason);

	/// <summary>结束传输，请求下位机校验并激活</summary>
	void otaRequestFinish();

	/// <summary>复位升级状态（不发信号）</summary>
	void otaResetState();

	/// <summary>升级成功后重新查询下位机版本。
	/// attempt 为已尝试次数：首次查询若在超时后仍拿不到版本，会再试一次
	/// （下位机刚复位，第一次查询可能撞上它的启动过程而丢失）</summary>
	void requeryVersionAfterOta(int attempt = 0);

	// 升级状态
	/// <summary>升级阶段。用它而不是几个 bool 组合，是因为"应用受理 → 复位 →
	/// 引导程序接管 → 建立会话 → 传输 → 校验"这条链上每一步期待的应答都不同，
	/// 用布尔量很容易把"应用受理"误当成"引导程序就绪"（这个坑已踩过：
	/// 应用回 OTAOK 后立刻开始传数据，而那时引导程序还没起来，于是每一帧都被
	/// 拒收 E.xx，重试 5 次后升级中止）</summary>
	enum class OtaPhase {
		Idle,        ///< 未在升级
		EnterApp,    ///< 已发 OTA\n，等**应用**受理（应用回 OTAOK 后置标志并软复位）
		WaitBoot,    ///< 应用已受理，等复位完成（轮询 V\n，收到 V0.x 说明引导程序已接管）
		EnterBoot,   ///< 已向**引导程序**发 OTA\n，等它回 OTAOK（会话建立后才收数据帧）
		Streaming,   ///< 正在下发数据帧
		Finishing,   ///< 已发 OTAE\n，等最终校验结果
	};

	QByteArray m_otaImage;          // 固件镜像
	qint64 m_otaSent = 0;           // 已确认字节数
	uint8_t m_otaSeq = 0;           // 帧序号
	int m_otaRetry = 0;             // 当前帧已重发次数
	bool m_otaActive = false;       // 升级流程进行中
	bool m_otaWaitingAck = false;   // 正在等待某帧应答
	bool m_otaFinishing = false;    // 已发出结束指令，等待最终应答
	OtaPhase m_otaPhase = OtaPhase::Idle;
	/// <summary>当前帧负载长度。初始为 kOtaChunkMax；同一帧反复被拒时逐级下调
	/// （帧长超过 BLE MTU 时下位机会收到残帧 → CRC 不过 → 回 E.xx）</summary>
	int m_otaChunk = kOtaChunkMax;
	/// <summary>最近一帧的实际负载长度，应答确认时按它推进进度（不能用常量算）</summary>
	int m_otaLastTake = 0;
	/// <summary>等待引导程序就绪的轮询计数</summary>
	int m_otaBootPollCount = 0;
	QTimer* m_otaAckTimer = nullptr;
	/// <summary>复位等待期的轮询定时器：周期性发 V\n 判断引导程序是否已接管</summary>
	QTimer* m_otaBootPollTimer = nullptr;
	/// <summary>otaResponseLine → handleOtaResponse 是否已连接。
	/// ⚠️ 必须是**实例成员**而不是函数内 static：static 会在第一个实例上置位，
	/// 之后新建的 BLEThread 实例（模块重建）就再也不会连接，表现为"升级没反应"</summary>
	bool m_otaWired = false;

	/// <summary>取消订阅（重连/断连/卸载前调用）</summary>
	void unsubscribeDeviceStatus();

	/// <summary>订阅/退订与写入结果的辅助：确认上一次写入的真实结果。
	/// 写入是异步的，"请求已受理"不等于"真的写成功"，因此稍后轮询一次真实结果</summary>
	void confirmLastWrite();

	/// <summary>指令被拒后安排一次自动重发（同一轮下发最多重试一次）</summary>
	void scheduleShutdownRetry();

	/// <summary>按"下位机自报的剩余秒数"安排一次散热结束核对：
	/// 中途断连会丢掉 FIN 回传，靠它兜底给用户提示</summary>
	void scheduleShutdownFinishedCheck(int seconds);

	bool loadBleLibrary();
	void safeUnloadLibrary();
};