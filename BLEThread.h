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

// DLL 函数指针类型
using BleHandle = void*;

using ScannedBleDeviceCallback = void(*)(void* device, const char* name, const char* address, const char* deviceId, int rssi);

using CreateBleHandleFunc = BleHandle(*)();
using DestroyBleHandleFunc = void(*)(BleHandle);
using StartBleScanFunc = void(*)(BleHandle, ScannedBleDeviceCallback); // 参数改为 BleHandle（void*）
using StopBleScanFunc = void(*)(BleHandle);
using ConnectBleDeviceFunc = bool(*)(BleHandle, const char* deviceId);
using DisconnectBleDeviceFunc = void(*)(BleHandle);
using CheckCharacteristicWritePermissionfunc = bool(*)(BleHandle, unsigned int serviceUUID, unsigned int characteristicUUID);
using WriteBleCharacteristicFunc = bool(*)(BleHandle, unsigned int ServiceUUID, unsigned int CharacteristicUUID, const unsigned char* buffer, unsigned int length, bool requireResponse);
using IsConnectedFunc = bool(*)(BleHandle);

// 前向声明（回调实现在 .cpp 中，通过 s_activeScanner 路由到实例）
void bleDeviceScannedCallback(void* device, const char* name, const char* address, const char* deviceId, int rssi);
void bleVerifyDeviceCallback(void* device, const char* name, const char* address, const char* deviceId, int rssi);

class BLEThread : public QObject, public IAppModule {
	Q_OBJECT

	// DLL 回调友元（通过 s_activeScanner 路由到实例）
	friend void bleDeviceScannedCallback(void*, const char*, const char*, const char*, int);
	friend void bleVerifyDeviceCallback(void*, const char*, const char*, const char*, int);

public:
	explicit BLEThread(IConfigProvider* config, QObject* parent = nullptr);
	~BLEThread();

	// --- IAppModule 接口 ---
	bool initialize() override;
	void start() override;
	void stop(int timeoutMs = 3000) override;

public slots:
	void run();

	/// <summary>
	/// 设备重连
	/// </summary>
	void reconnectDevice();

	/// <summary>
	/// 检测蓝牙连接状态
	/// </summary>
	void updateConnectionStatus();

	/// <summary>
	/// 风扇转速控制
	/// </summary>
	/// <param name="buff"></param>
	void controlFan(char* buff);

	/// <summary>
	/// 自动模式
	/// </summary>
	void autoMode();

	/// <summary>
	/// 静音模式
	/// </summary>
	void silentMode();

	/// <summary>
	/// 全速模式
	/// </summary>
	void performanceMode();

signals:
	/// <summary>
	///  扫描开始
	/// </summary>
	void bleScanStarted();

	/// <summary>
	/// 扫描超时
	/// </summary>
	void bleScanTimeout();

	/// <summary>
	/// 连接中
	/// </summary>
	void connectionInProgress();

	/// <summary>
	/// 连接失败
	/// </summary>
	void connectionFailed();

	/// <summary>
	/// 连接成功
	/// </summary>
	void connected();

	/// <summary>
	/// 断开连接
	/// </summary>
	void disconnected();

	/// <summary>
	/// 日志数据
	/// </summary>
	/// <param name="message"> 日志消息 </param>
	/// <param name="level"> 日志级别 </param>
	void logMessage(const QString& message, LogManagement::LogLevel level);

private:
	/// <summary>BLE DLL 函数指针表（实例级别，替代全局静态变量）</summary>
	struct BLEFunctionTable {
		CreateBleHandleFunc createHandle = nullptr;
		DestroyBleHandleFunc destroyHandle = nullptr;
		StartBleScanFunc startScan = nullptr;
		StopBleScanFunc stopScan = nullptr;
		ConnectBleDeviceFunc connectDevice = nullptr;
		DisconnectBleDeviceFunc disconnectDevice = nullptr;
		CheckCharacteristicWritePermissionfunc checkCharacteristicWritePermission = nullptr;
		WriteBleCharacteristicFunc writeDataByCharacteristic = nullptr;
		IsConnectedFunc isConnected = nullptr;
	};

	/// <summary>扫描上下文（替代全局 g_name / g_deviceId / g_isFind）</summary>
	struct ScanContext {
		QString targetName;
		QString foundDeviceId;
		std::atomic<bool> isFound{ false };
	};

	struct BleDeviceInfo {
		QString id; // 设备ID
		QString name; // 设备名称
		quint16 serviceUuid = 0xFFE0; // 服务UUID
		quint16 characteristicUuid = 0xFFE1; // 特征UUID
		int retryCount = 0; // 重试次数
		bool isFind = false; // 是否找到
		bool isConnected = false; // 是否已连接
	};

	BleHandle m_bleHandle = nullptr;
	NativeLibraryLoader m_bleLoader;
	BLEFunctionTable m_bleFuncs; // 实例级 DLL 函数指针
	ScanContext m_scanCtx;       // 实例级扫描上下文
	BleDeviceInfo m_deviceInfo;
	QMutex m_operationMutex;
	QMutex m_fanControlMutex;
	QMutex m_modeMutex;
	IConfigProvider* m_config; // 配置提供者（依赖注入）
	QThread* m_workThread = nullptr; // 自管理的工作线程（仅当作为独立模块时使用）

	// 并发控制
	std::atomic<bool> m_bleInitialized{ false }; // BLE 已初始化（防止重复 initializeBle）
	std::atomic<bool> m_connecting{ false };     // 正在连接中（防止 updateConnectionStatus 干扰）

	/// <summary>
	/// 初始化配置文件
	/// </summary>
	void initializeIniFile();

	/// <summary>
	/// 加载设备配置文件
	/// </summary>
	void loadDeviceConfig();

	/// <summary>
	/// 扫面设备
	/// </summary>
	void scanDevices();
	void scanVerifyDevice();
	void scanDevicesInternal(ScannedBleDeviceCallback callback, bool setId);

	/// <summary>
	/// 初始化蓝牙
	/// </summary>
	void initializeBle();

	/// <summary>
	/// 蓝牙首次链接
	/// </summary>
	void performFirstConnection();

	/// <summary>
	/// 蓝牙非第一次链接
	/// </summary>
	/// <param name="deviceId"></param>
	void connectToDevice(const QString& deviceId);

	/// <summary>
	/// 蓝牙连接状态
	/// </summary>
	void ConnectionStatus();

	/// <summary>
	/// 蓝牙发送数据
	/// </summary>
	/// <param name="data"></param>
	void sendData(const unsigned char* buffer, size_t length);

	/// <summary>发送风扇模式（统一 buffer 构造逻辑）</summary>
	void sendFanMode(FanMode mode);

	/// <summary>
	/// 加载BLE Dll库
	/// </summary>
	/// <returns></returns>
	bool loadBleLibrary();

	/// <summary>
	/// 卸载BLE　Dll库
	/// </summary>
	void safeUnloadLibrary();
};