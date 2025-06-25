#pragma once

#include <QObject>
#include <QLibrary>
#include <QDebug>
#include <QMessageBox>
#include <windows.h>
#include <QApplication>
#include <QTimer>

#include <thread>
#include <algorithm>
#include <cmath>

#include <IniManagement.h>
#include <LogManagement.h>
#include "BLEThread.h"

class TCCore : public QObject
{
	Q_OBJECT

public:
	explicit TCCore(QObject* parent = nullptr);
	~TCCore();

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

	LogManagement* m_logTCCore; //日志管理器

	bool m_setDelayFlag; //设置延时标志

	BLEThread* m_bleThread; //蓝牙线程

	/*-----------------------------------------------*/
	const int  InitCpuTemp = 40;	//初始CPU温度等级
	const int  InitGpuTemp = 40;	//初始GPU温度等级
	const int cpuStep = 10;			//CPU温度步长
	const int gpuStep = 8;			//GPU温度步长
	int cpuLevel;
	int gpuLevel;
	int historyLevel[5] = { 0,0,0,0,0 }; //历史数据等级
	float weights[5] = { 1, 1, 2, 3, 2 }; // 自定义权重（中心权重高）

	int sendHistoryLevel;	// 发送的历史等级
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
	enum DllLoadStatus {
		DLL_NOT_LOADED = 0, //DLL未加载
		DLL_LOADED = 1,	//DLL已加载
		DLL_LOAD_FAILED = -1, //DLL加载失败
		DLL_RESOLVE_FAILED = -2 //DLL解析失败
	};

	enum CpuType {
		CPU_INTEL,
		CPU_AMD,
		CPU_UNKNOWN
	};

	typedef char* (*GetCPUType_Func)();
	typedef void (*FreeCPUType_Func)(char*);

	typedef int (*Intel_Init_Func)();
	typedef int (*Intel_GetTemp_Func)();

	typedef int (*AMD_Init_Func)();
	typedef int (*AMD_GetTemp_Func)();

	typedef int (*Nv_Init_Func)();
	typedef int (*Nv_GetTemp_Func)();

	/*-------------------------------------------------------------------------*/

	/// <summary>
	/// DLL加载状态
	/// </summary>
	struct Dll_Status {
		DllLoadStatus CPUID_Status = DLL_NOT_LOADED; //CPUID DLL加载状态
		DllLoadStatus Intel_Status = DLL_NOT_LOADED; //Intel DLL加载状态
		DllLoadStatus AMD_Status = DLL_NOT_LOADED; //AMD DLL加载状态
		DllLoadStatus Nv_Status = DLL_NOT_LOADED; //NVIDIA DLL加载状态
		CpuType CPUType = CPU_UNKNOWN; //CPU类型
	};
	Dll_Status m_dllStatus;

	/// <summary>
	/// DLL 管理器状态
	/// </summary>
	struct Mgmt_Flage {
		bool Load_CPU_Dll_Mgmt_Flage = false; //CPU DLL加载管理器标志
		bool Load_GPU_Dll_Mgmt_Flage = false; //GPU DLL加载管理器标志
	};
	Mgmt_Flage m_mgmtFlag;

	//struct ControlData {
	//	int sendData = 0; //最早的数据
	//	int lastData = 0; //上次的数据
	//	//int historyData = 0; //历史数据
	//};
	//ControlData m_controlData;

	QLibrary m_cpuidLib; //CPUID DLL
	QLibrary m_intelLib; //Intel DLL
	QLibrary m_amdLib; //AMD DLL
	QLibrary m_nvLib; //NVIDIA DLL

	GetCPUType_Func m_getCPUType;
	FreeCPUType_Func m_freeCPUType;

	Intel_Init_Func m_intelInit;
	Intel_GetTemp_Func m_intelGetTemp;

	AMD_Init_Func m_amdInit;
	AMD_GetTemp_Func m_amdGetTemp;

	Nv_Init_Func m_nvInit;
	Nv_GetTemp_Func m_nvGetTemp;

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

	/*-----------------------------------------------------------------*/
	///// <summary>
	///// DLL库状态
	///// </summary>
	//struct Dll_Status {
	//	int	CPUID_Status = 0;
	//	int	Intel_Status = 0;
	//	int	AMD_Status = 0;
	//	int	Nv_Status = 0;
	//	char* CPUID;
	//};
	//Dll_Status DllStatus;

	///// <summary>
	///// DLL库管理器状态
	///// </summary>
	//struct Mgmt_Flage {
	//	int Load_CPU_Dll_Mgmt_Flage = 0;
	//	int Load_GPU_Dll_Mgmt_Flage = 0;
	//};
	//Mgmt_Flage MgmtFlage;

	//QLibrary CPUID_lib;
	//QLibrary Intel_lib;
	//QLibrary AMD_lib;
	//QLibrary Nv_lib;

	//GetCPUType_Func GetCPUType;
	//FreeCPUType_Func FreeCPUType;

	//Intel_Init_Func Intel_Init;
	//Intel_GetTemp_Func Intel_GetTemp;

	//AMD_Init_Func AMD_Init;
	//AMD_GetTemp_Func AMD_GetTemp;

	//Nv_Init_Func Nv_Init;
	//Nv_GetTemp_Func Nv_GetTemp;

	//// 加载CPUID DLL
	//void LOAD_CPUID_DLL();
	//// 加载Intel DLL
	//void LOAD_Intel_DLL();
	//// 加载AMD DLL
	//void LOAD_AMD_DLL();
	//// 加载Nv DLL
	//void LOAD_Nv_DLL();

	//// 加载CPU DLL管理
	//void LOAD_CPU_DLL_Mgmt();
	//// 加载GPU DLL管理
	//void LOAD_GPU_DLL_Mgmt();
	//// 获取CPU温度管理
	//int Get_CPUTemp_Mgmt();
	//// 获取GPU温度管理
	//int Get_GPUTemp_Mgmt();

	/*-----------------------------------------------------------------------------*/
};
