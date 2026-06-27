// BLEThread.h
#pragma once

#include <QObject>
#include <QMutex>
#include <QThread>
#include <QTimer>

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

// WinRT_BLE_DLL 函数指针类型
using BleInitializeFunc = int(*)();
using BleUninitializeFunc = void(*)();
using BleStartScanFunc = int(*)(BleScanCallback, void*);
using BleStopScanFunc = void(*)(int);
using BleConnectFunc = int(*)(const wchar_t*, BleConnectionCallback, void*);
using BleDisconnectFunc = void(*)(int);
using BleIsConnectedFunc = int(*)(int);
using BleWriteCharacteristicFunc = BleError(*)(int, const wchar_t*, const wchar_t*, const uint8_t*, uint32_t);
using BleGetLastErrorFunc = BleError(*)();
using BleErrorToStringFunc = const wchar_t*(*)(BleError);

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
		BleGetLastErrorFunc getLastError = nullptr;
		BleErrorToStringFunc errorToString = nullptr;
	};

	/// <summary>扫描上下文</summary>
	struct ScanContext {
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
		bool isFind = false;
		bool isConnected = false;
	};

	// 回调（静态方法，通过 userData 路由到实例）
	static void onDeviceScanned(const wchar_t* address, const wchar_t* name, int16_t rssi, void* userData);
	static void onConnectionChanged(const wchar_t* address, int connected, const wchar_t* error, void* userData);

	// 内部连接等待回调
	static void onFirstConnResult(const wchar_t* address, int connected, const wchar_t* error, void* userData);

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

	/// <summary>将 16 位 UUID 整数转为宽字符串（如 0xFFE0 → "FFE0"）</summary>
	static QString uuidToString(quint16 uuid);

	/// <summary>将 BleError 转为带排查建议的日志消息</summary>
	QString bleErrorToMessage(BleError err) const;

	void initializeIniFile();
	void loadDeviceConfig();

	void scanDevices();
	void scanDevicesRetry();
	void scanVerifyDevice();
	void scanVerifyDeviceRetry();
	void scanDevicesInternal(bool matchByName, bool setId, bool emitStarted);

	void initializeBle();

	void performFirstConnection();
	void performFirstConnectionRetry();
	void connectToDevice(const QString& address);
	void connectToDeviceRetry(const QString& address);

	/// <summary>异步连接并等待回调结果</summary>
	bool connectAndWait(const QString& address);

	void ConnectionStatus();
	void sendData(const unsigned char* buffer, size_t length);
	void sendFanMode(FanMode mode);

	bool loadBleLibrary();
	void safeUnloadLibrary();
};