// BLEThread.h
#pragma once

#include <QObject>
#include <QLibrary>
#include <QMutex>
#include <QSettings>
#include <QTimer>
#include <QEventLoop>

#include "LogManagement.h"
#include "IniManagement.h"

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

void OnDeviceScannedStatic(void* device, const char* name, const char* address, const char* deviceId, int rssi);
void scanVerifyDeviceStatic(void* device, const char* name, const char* address, const char* deviceId, int rssi);

class BLEThread : public QObject {
	Q_OBJECT

public:
	explicit BLEThread(QObject* parent = nullptr);
	~BLEThread();

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
	struct BleDeviceInfo {
		QString id; // 设备ID
		QString name; // 设备名称
		quint16 serviceUuid = 0xFFE0; // 服务UUID
		quint16 characteristicUuid = 0xFFE1; // 特征UUID
		int retryCount = 0; // 重试次数
		bool isFind = false; // 是否找到
		bool isConnected = false; // 是否已连接
	};

	QLibrary m_bleLibrary;
	BleHandle m_bleHandle = nullptr;
	BleDeviceInfo m_deviceInfo;
	QMutex m_operationMutex;
	QMutex m_fanControlMutex;
	QMutex m_modeMutex;
	LogManagement* m_logger;

	enum class DllState { NotLoaded, Loaded, Failed };
	DllState m_dllState = DllState::NotLoaded;

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

	/// <summary>
	/// 扫描验证设备
	/// </summary>
	void scanVerifyDevice();

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
	void sendData(const unsigned char* buffer);

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