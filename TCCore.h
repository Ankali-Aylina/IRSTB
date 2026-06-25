#pragma once

#include <QObject>
#include <QTimer>
#include <QThread>

#include <algorithm>
#include <cmath>
#include "LogManagement.h"
#include "IAppModule.h"
#include "IConfigProvider.h"
#include "NativeLibraryLoader.h"
#include "TemperatureConfig.h"

class TCCore : public QObject, public IAppModule
{
	Q_OBJECT

public:
	explicit TCCore(IConfigProvider* config, QObject* parent = nullptr);
	~TCCore();

	// --- IAppModule 接口 ---
	bool initialize() override;
	void start() override;
	void stop(int timeoutMs = 3000) override;

public slots:

	/// <summary>
	/// 运行TCCore
	/// </summary>
	void run();

	//获取CPU
	int getCPUTemp();

	//获取GPU温度
	int getGPUTemp();

	// 获取控制数据
	void getControlData();

	/// <summary>
	/// 设置数据发送定时间标志
	/// </summary>
	/// <param name="flag"></param>
	void setDataTrDelayUpdataFlag(bool flag);

signals:

	/// <summary>
	/// 日志信号
	/// </summary>
	/// <param name="message"></param>
	/// <param name="level"></param>
	void logMessage(const QString& message, LogManagement::LogLevel level);

	/// <summary>
	/// 连接状态信号
	/// </summary>
	void updateConnectionStatus();

	/// <summary>
	/// CPU温度信号
	/// </summary>
	void cpuTemperatureUpdated(int temperature);

	/// <summary>
	/// GPU温度信号
	/// </summary>
	/// <param name="temperature"> 温度值 </param>
	void gpuTemperatureUpdated(int temperature);

	/// <summary>
	/// 定义控制数据信号
	/// </summary>
	/// <param name="data"></param>
	void controlDataUpdated(char* data);

private:
	std::atomic<bool> m_stopped{ false };  // 添加停止标志

	IConfigProvider* m_config; // 配置提供者（依赖注入）

	QThread* m_workThread = nullptr; // 自管理的工作线程

	bool m_setDelayFlag; //设置延时标志

	TemperatureConfig m_tempCfg; // 温度阈值配置（可从 INI 加载）

	int cpuLevel;
	int gpuLevel;
	std::vector<int> historyLevel;   // 动态历史等级
	std::vector<float> tempWeights;  // 动态权重（副本，避免跨线程访问配置）

	int sendHistoryLevel;	// 上次发送的等级
	int m_consecutiveLow = 0; // 连续低于当前等级的计数（滞后）
	bool sendHistoryFirst = true;	// 首次发送标志

	/// <summary>
	/// 计算CPU温度等级
	/// </summary>
	void cpuTempLevel();

	/// <summary>
	/// 计算GPU温度等级
	/// </summary>
	void gpuTempLevel();

	/// <summary>
	/// 计算历史数据等级
	/// </summary>
	/// <returns></returns>
	int TempMgmt();

	/*------------------------------------------------------*/

	enum CpuType {
		CPU_INTEL,
		CPU_AMD,
		CPU_UNKNOWN
	};

	typedef char* (*GetCPUType_Func)();
	typedef void (*FreeCPUType_Func)(char*);

	typedef int (*AMD_Init_Func)();
	typedef int (*AMD_GetTemp_Func)();

	// --- IntelTemp DLL API (替代旧 intel_dll.dll) ---
	// 温度类型
	enum TemperatureType { TEMP_TYPE_CORE = 0, TEMP_TYPE_PACKAGE = 1 };

	// 函数指针
	typedef int (*IntelTemp_Initialize_Func)(const wchar_t* binaryPath);
	typedef int (*IntelTemp_ReadTemp_Func)(int type, float* temp);
	typedef int (*IntelTemp_GetLastError_Func)(char* detail, int detailSize);
	typedef void (*IntelTemp_Cleanup_Func)();

	typedef int (*Nv_Init_Func)();
	typedef int (*Nv_GetTemp_Func)();

	/*-------------------------------------------------------------------------*/

	CpuType m_cpuType = CPU_UNKNOWN; // CPU类型

	NativeLibraryLoader m_cpuidLoader;
	NativeLibraryLoader m_intelLoader;
	NativeLibraryLoader m_amdLoader;
	NativeLibraryLoader m_nvLoader;

	bool m_cpuDllLoaded = false; // CPU DLL 管理标志
	bool m_gpuDllLoaded = false; // GPU DLL 管理标志

	GetCPUType_Func m_getCPUType;
	FreeCPUType_Func m_freeCPUType;

	// IntelTemp API 函数指针
	IntelTemp_Initialize_Func m_inteltempInit = nullptr;
	IntelTemp_ReadTemp_Func m_inteltempReadTemp = nullptr;
	IntelTemp_GetLastError_Func m_inteltempGetLastError = nullptr;
	IntelTemp_Cleanup_Func m_inteltempCleanup = nullptr;
	bool m_inteltempInitialized = false; // 是否已调用 inteltemp_initialize

	AMD_Init_Func m_amdInit;
	AMD_GetTemp_Func m_amdGetTemp;
	bool m_amdInitialized = false; // AMD Init 已调用

	Nv_Init_Func m_nvInit;
	Nv_GetTemp_Func m_nvGetTemp;
	bool m_nvInitialized = false; // NV Init 已调用

	/// <summary>
	/// 加载CPUID DLL
	/// </summary>
	void loadCPUIDLib();

	/// <summary>
	/// 加载Intel DLL
	/// </summary>
	void loadIntelLib();

	/// <summary>
	/// 加载AMD DLL
	/// </summary>
	void loadAMDLib();

	/// <summary>
	/// 加载NVIDIA DLL
	/// </summary>
	void loadNvLib();

	/// <summary>
	/// 加载CPU DLL管理
	/// </summary>
	void loadCpuDllMgmt();
	/// <summary>
	/// 加载GPU DLL管理
	/// </summary>
	void loadGpuDllMgmt();

	/// <summary>
	/// 获取CPU温度管理
	/// </summary>
	/// <returns> CPU温度 </returns>
	int getCpuTempMgmt();

	/// <summary>
	/// 获取GPU温度管理
	/// </summary>
	/// <returns> GPU温度 </returns>
	int getGpuTempMgmt();

	/// <summary>
	/// 初始化配置文件
	/// </summary>
	void initConfigFile();

	/// <summary>
	/// 获取数据发送延迟
	/// </summary>
	int getDataTrDelay();
};
