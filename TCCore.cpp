#include "TCCore.h"

TCCore::TCCore(QObject* parent)
	: QObject(parent)
{
	// 初始化函数指针
	m_getCPUType = nullptr;
	m_freeCPUType = nullptr;
	m_intelInit = nullptr;
	m_intelGetTemp = nullptr;
	m_amdInit = nullptr;
	m_amdGetTemp = nullptr;
	m_nvInit = nullptr;
	m_nvGetTemp = nullptr;

	m_logTCCore = new LogManagement(this);
	connect(this, &TCCore::logMessage, m_logTCCore, &LogManagement::logMessage);

	m_bleThread = new BLEThread();
	connect(this, &TCCore::updateConnectionStatus, m_bleThread, &BLEThread::updateConnectionStatus);
}

TCCore::~TCCore()
{
	if (m_cpuidLib.isLoaded()) m_cpuidLib.unload();
	if (m_intelLib.isLoaded()) m_intelLib.unload();
	if (m_amdLib.isLoaded()) m_amdLib.unload();
	if (m_nvLib.isLoaded()) m_nvLib.unload();
}

void TCCore::run()
{
	initConfigFile();
	int TxDelay = getDataTrDelay();

	QEventLoop loop;
	QTimer timer, m_timer;
	timer.setSingleShot(true);
	timer.setInterval(TxDelay);

	connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

	while (1) {
		timer.start();

		getControlData();
		emit updateConnectionStatus();
		loop.exec();

		if (m_setDelayFlag) {
			timer.stop();
			TxDelay = getDataTrDelay();
			timer.setInterval(TxDelay);
			m_setDelayFlag = false;
		}
	}
}

int TCCore::getCPUTemp()
{
	int temp = getCpuTempMgmt();

	if (temp > 0) { // 添加有效性检查
		emit cpuTemperatureUpdated(temp); // 发射信号
	}
	emit logMessage(QString("CPU温度：%1°C").arg(temp), LogManagement::LogLevel::LOG_DEBUG);
	return temp;
}

int TCCore::getGPUTemp()
{
	int temp = getGpuTempMgmt();
	if (temp > 0) { // 添加有效性检查
		emit gpuTemperatureUpdated(temp); // 发射信号
	}
	emit logMessage(QString("GPU温度：%1°C").arg(temp), LogManagement::LogLevel::LOG_DEBUG);
	return temp;
}

void TCCore::cpuTempLevel()
{
	int cpuTemp = getCPUTemp();
	if (cpuTemp < InitCpuTemp + cpuStep * 1)
	{
		cpuLevel = 0;
	}
	else if (cpuTemp < InitCpuTemp + cpuStep * 2)
	{
		cpuLevel = 1;
	}
	else if (cpuTemp < InitCpuTemp + cpuStep * 3)
	{
		cpuLevel = 2;
	}
	else if (cpuTemp < InitCpuTemp + cpuStep * 4)
	{
		cpuLevel = 3;
	}
	else
	{
		cpuLevel = 4;
	}
}

void TCCore::gpuTempLevel()
{
	int gpuTemp = getGPUTemp();

	if (gpuTemp < InitGpuTemp + gpuStep * 1)
	{
		gpuLevel = 0;
	}
	else if (gpuTemp < InitGpuTemp + gpuStep * 2)
	{
		gpuLevel = 1;
	}
	else if (gpuTemp < InitGpuTemp + gpuStep * 3)
	{
		gpuLevel = 2;
	}
	else if (gpuTemp < InitGpuTemp + gpuStep * 4)
	{
		gpuLevel = 3;
	}
	else
	{
		gpuLevel = 4;
	}
}

int TCCore::TempMgmt()
{
	int level = 0;

	//启动两个线程，分别计算CPU和GPU的温度等级
	std::thread cl([this]() {
		this->cpuTempLevel(); // 显式调用成员函数
		});
	std::thread gl([this]() {
		this->gpuTempLevel(); // 显式调用成员函数
		});
	cl.join();
	gl.join();

	// 计算平均等级，加权计算
	const int maxLevel = qMax(cpuLevel, gpuLevel);

	// 更新历史等级（数组左移）
	std::rotate(historyLevel, historyLevel + 1, historyLevel + 5);

	// 将当前等级添加到历史等级数组的末尾
	historyLevel[4] = maxLevel;

	// 计算加权平均等级
	double sum = 0;
	for (int i = 0; i < 5; i++) {
		sum += historyLevel[i] * weights[i];
	}
	sum /= 5;

	// 四舍五入取整
	level = std::round(sum);
	return level;
}

void TCCore::getControlData()
{
	static char buffer[2]; // 静态缓冲区避免悬空指针
	//char* buffer;
	int value = TempMgmt();

	if (value != sendHistoryLevel) {
		sendHistoryLevel = value;
		snprintf(buffer, sizeof(buffer), "%d", value);
		emit controlDataUpdated(buffer);

		qDebug() << "发送数据：" << buffer;
	}
	else
	{
		if (sendHistoryFirst)
		{
			snprintf(buffer, sizeof(buffer), "%d", value);
			emit controlDataUpdated(buffer);
			sendHistoryFirst = false;

			//qDebug() << "首次发送数据：" << buffer;
		}
	}
	qDebug() << "当前等级：" << value;
}

void TCCore::setDataTrDelayUpdataFlag(bool flag)
{
	m_setDelayFlag = flag;
}

void TCCore::loadCPUIDLib()
{
	if (m_dllStatus.CPUID_Status == DLL_LOADED)return;

	m_cpuidLib.setFileName("Cpu_Dll.dll");
	if (!m_cpuidLib.load()) {
		m_dllStatus.CPUID_Status = DLL_LOAD_FAILED;
		emit logMessage(QString("CPU信息库加载失败!"), LogManagement::LogLevel::LOG_ERROR);
		return;
	}

	m_getCPUType = (GetCPUType_Func)m_cpuidLib.resolve("get_cpu_type");
	m_freeCPUType = (FreeCPUType_Func)m_cpuidLib.resolve("free_cpu_type");
	if (!m_getCPUType || !m_freeCPUType) {
		m_dllStatus.CPUID_Status = DLL_RESOLVE_FAILED;
		emit logMessage(QString("CPU信息库函数解析失败!"), LogManagement::LogLevel::LOG_ERROR);
		return;
	}

	// 获取CPU类型并解析
	char* cpuType = m_getCPUType();
	QString typeStr = QString(cpuType);
	emit logMessage(QString("CPU类型：" + typeStr), LogManagement::LogLevel::LOG_INFO);
	m_freeCPUType(cpuType);

	if (typeStr.contains("Intel")) {
		m_dllStatus.CPUType = CPU_INTEL;
	}
	else if (typeStr.contains("AMD")) {
		m_dllStatus.CPUType = CPU_AMD;
	}
	else {
		m_dllStatus.CPUType = CPU_UNKNOWN;
	}

	m_dllStatus.CPUID_Status = DLL_LOADED;
}

void TCCore::loadIntelLib()
{
	if (m_dllStatus.Intel_Status == DLL_LOADED)return;
	m_intelLib.setFileName("intel_dll.dll");
	if (!m_intelLib.load()) {
		m_dllStatus.Intel_Status = DLL_LOAD_FAILED;
		emit logMessage(QString("Intel信息库加载失败!"), LogManagement::LogLevel::LOG_ERROR);
		return;
	}
	m_intelInit = (Intel_Init_Func)m_intelLib.resolve("intel_init");
	m_intelGetTemp = (Intel_GetTemp_Func)m_intelLib.resolve("get_cpu_temperature");
	if (!m_intelInit || !m_intelGetTemp) {
		m_dllStatus.Intel_Status = DLL_RESOLVE_FAILED;
		emit logMessage(QString("Intel信息库函数解析失败!"), LogManagement::LogLevel::LOG_ERROR);
		return;
	}
	m_dllStatus.Intel_Status = DLL_LOADED;
}

void TCCore::loadAMDLib()
{
	if (m_dllStatus.AMD_Status == DLL_LOADED)return;
	m_amdLib.setFileName("amd_dll.dll");
	if (!m_amdLib.load()) {
		m_dllStatus.AMD_Status = DLL_LOAD_FAILED;
		emit logMessage(QString("AMD信息库加载失败!"), LogManagement::LogLevel::LOG_ERROR);
		return;
	}
	m_amdInit = (AMD_Init_Func)m_amdLib.resolve("amd_init");
	m_amdGetTemp = (AMD_GetTemp_Func)m_amdLib.resolve("get_amd_temperature");
	if (!m_amdInit || !m_amdGetTemp) {
		m_dllStatus.AMD_Status = DLL_RESOLVE_FAILED;
		emit logMessage(QString("AMD信息库函数解析失败!"), LogManagement::LogLevel::LOG_ERROR);
		return;
	}
	m_dllStatus.AMD_Status = DLL_LOADED;
}

void TCCore::loadNvLib()
{
	if (m_dllStatus.Nv_Status == DLL_LOADED)return;
	m_nvLib.setFileName("nv_dll.dll");
	if (!m_nvLib.load()) {
		m_dllStatus.Nv_Status = DLL_LOAD_FAILED;
		emit logMessage(QString("NVIDIA信息库加载失败!"), LogManagement::LogLevel::LOG_ERROR);
		return;
	}
	m_nvInit = (Nv_Init_Func)m_nvLib.resolve("nv_init");
	m_nvGetTemp = (Nv_GetTemp_Func)m_nvLib.resolve("nv_get_temperature");
	if (!m_nvInit || !m_nvGetTemp) {
		m_dllStatus.Nv_Status = DLL_RESOLVE_FAILED;
		emit logMessage(QString("NVIDIA信息库函数解析失败!"), LogManagement::LogLevel::LOG_ERROR);
		return;
	}
	m_dllStatus.Nv_Status = DLL_LOADED;
}

void TCCore::loadCpuDllMgmt()
{
	if (m_mgmtFlag.Load_CPU_Dll_Mgmt_Flage != false) return;
	loadCPUIDLib();
	if (m_dllStatus.CPUType == CPU_INTEL) {
		loadIntelLib();
	}
	else if (m_dllStatus.CPUType == CPU_AMD) {
		loadAMDLib();
	}
	else {
		emit logMessage(QString("CPU类型未知"), LogManagement::LogLevel::LOG_ERROR);
	}
	m_mgmtFlag.Load_CPU_Dll_Mgmt_Flage = true;
}

void TCCore::loadGpuDllMgmt()
{
	if (m_mgmtFlag.Load_GPU_Dll_Mgmt_Flage != false) return;
	loadNvLib();
	m_mgmtFlag.Load_GPU_Dll_Mgmt_Flage = true;
}

int TCCore::getCpuTempMgmt()
{
	if (!m_mgmtFlag.Load_CPU_Dll_Mgmt_Flage) {
		loadCpuDllMgmt();
	}

	switch (m_dllStatus.CPUType) {
	case CPU_INTEL:
		m_intelInit();
		return m_intelGetTemp ? m_intelGetTemp() : -99;
	case CPU_AMD:
		m_amdInit();
		return m_amdGetTemp ? m_amdGetTemp() : -99;
	default:
		emit logMessage(QString("错误，不支持的CPU类型"), LogManagement::LogLevel::LOG_ERROR);
		return -100;
	}
}

int TCCore::getGpuTempMgmt()
{
	if (!m_mgmtFlag.Load_GPU_Dll_Mgmt_Flage) {
		loadGpuDllMgmt();
	}
	m_nvInit();
	return m_nvGetTemp ? m_nvGetTemp() : -99;
}

void TCCore::initConfigFile()
{
	if (!IniManagement::instance().IsInit("TC"))
	{
		IniManagement::instance().InitSection("TC", "false");
		IniManagement::instance().write("TC", "DataTransmissionDelay", "5000");
		IniManagement::instance().InitSection("TC", "true");
	}
	return;
}

int TCCore::getDataTrDelay()
{
	return IniManagement::instance().read("TC", "DataTransmissionDelay").toInt();
}