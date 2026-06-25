#include "TCCore.h"
#include <QCoreApplication>

TCCore::TCCore(IConfigProvider* config, QObject* parent)
	: QObject(parent)
	, m_config(config)
	, m_setDelayFlag(false)
{
	// 函数指针初始化
	m_getCPUType = nullptr;
	m_freeCPUType = nullptr;
	m_amdInit = nullptr;
	m_amdGetTemp = nullptr;
	m_nvInit = nullptr;
	m_nvGetTemp = nullptr;
}

TCCore::~TCCore()
{
	stop();

	// 清理 IntelTemp
	if (m_inteltempInitialized && m_inteltempCleanup)
	{
		m_inteltempCleanup();
		m_inteltempInitialized = false;
	}

	m_cpuidLoader.unload();
	m_intelLoader.unload();
	m_amdLoader.unload();
	m_nvLoader.unload();
}

// --- IAppModule 接口实现 ---

bool TCCore::initialize()
{
	// 初始化配置文件
	initConfigFile();

	// 连接日志到全局单例
	connect(this, &TCCore::logMessage, &LogManagement::instance(), &LogManagement::logMessage);

	// 加载温度阈值配置
	m_tempCfg = TemperatureConfig::loadFromConfig(m_config);
	historyLevel.assign(m_tempCfg.historySize, 0);
	tempWeights = m_tempCfg.weights;

	// 预加载所有 DLL（在主线程执行，满足 inteltemp_initialize 等 API 的线程要求）
	loadCpuDllMgmt();
	loadGpuDllMgmt();

	return true;
}

void TCCore::start()
{
	if (m_workThread) return; // 防止重复启动

	m_workThread = new QThread(this);
	moveToThread(m_workThread);

	connect(m_workThread, &QThread::started, this, &TCCore::run, Qt::QueuedConnection);

	m_workThread->start();
}

void TCCore::stop(int timeoutMs)
{
	m_stopped = true;
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

void TCCore::run()
{
	int TxDelay = getDataTrDelay();

	QEventLoop loop;
	QTimer timer;
	timer.setSingleShot(true);
	timer.setInterval(TxDelay);

	connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

	while (!m_stopped) {
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

static int calcTempLevel(int temp, int initTemp, int step, int maxLevels)
{
	// 公式替代 if-else 链：level = (temp - initTemp) / step，限制在 [0, maxLevels-1]
	int level = (temp - initTemp) / step;
	if (level < 0) level = 0;
	if (level >= maxLevels) level = maxLevels - 1;
	return level;
}

void TCCore::cpuTempLevel()
{
	cpuLevel = calcTempLevel(getCPUTemp(),
		m_tempCfg.initCpuTemp, m_tempCfg.cpuStep, m_tempCfg.maxLevels);
}

void TCCore::gpuTempLevel()
{
	gpuLevel = calcTempLevel(getGPUTemp(),
		m_tempCfg.initGpuTemp, m_tempCfg.gpuStep, m_tempCfg.maxLevels);
}

int TCCore::TempMgmt()
{
	cpuTempLevel();
	gpuTempLevel();

	// 取 CPU/GPU 中较高等级
	const int maxLevel = qMax(cpuLevel, gpuLevel);

	// 更新滑动窗口
	std::rotate(historyLevel.begin(), historyLevel.begin() + 1, historyLevel.end());
	historyLevel.back() = maxLevel;

	// 加权平均（修正：除以权重和而非窗口大小）
	double sum = 0;
	double weightSum = 0;
	const int hSize = static_cast<int>(historyLevel.size());
	for (int i = 0; i < hSize; i++) {
		sum += historyLevel[i] * tempWeights[i];
		weightSum += tempWeights[i];
	}

	return static_cast<int>(std::round(sum / weightSum));
}

void TCCore::getControlData()
{
	int value = TempMgmt();

	// 滞后：升温立即响应，降温需连续 3 次低值才下调（避免风扇频繁跳变）
	if (value > sendHistoryLevel) {
		m_consecutiveLow = 0;
	} else if (value < sendHistoryLevel) {
		if (++m_consecutiveLow < 3) {
			value = sendHistoryLevel; // 暂不降级
		} else {
			m_consecutiveLow = 0;
		}
	} else {
		m_consecutiveLow = 0;
	}

	if (value != sendHistoryLevel || sendHistoryFirst)
	{
		sendHistoryLevel = value;
		sendHistoryFirst = false;

		static char buffer[8];
		snprintf(buffer, sizeof(buffer), "%d", value);
		emit controlDataUpdated(buffer);

		qDebug() << "发送数据：" << buffer;
	}

	qDebug() << "当前等级：" << value;
}

void TCCore::setDataTrDelayUpdataFlag(bool flag)
{
	m_setDelayFlag = flag;
}

void TCCore::loadCPUIDLib()
{
	if (m_cpuidLoader.isLoaded()) return;

	if (!m_cpuidLoader.load("Cpu_Dll.dll", {
		{"get_cpu_type", (void**)&m_getCPUType},
		{"free_cpu_type", (void**)&m_freeCPUType}
	}))
	{
		emit logMessage(QString("CPU信息库%1").arg(m_cpuidLoader.errorString()),
		                LogManagement::LogLevel::LOG_ERROR);
		return;
	}

	// 获取CPU类型并解析
	char* cpuType = m_getCPUType();
	QString typeStr = QString(cpuType);
	emit logMessage(QString("CPU类型：" + typeStr), LogManagement::LogLevel::LOG_INFO);
	m_freeCPUType(cpuType);

	if (typeStr.contains("Intel")) {
		m_cpuType = CPU_INTEL;
	}
	else if (typeStr.contains("AMD")) {
		m_cpuType = CPU_AMD;
	}
	else {
		m_cpuType = CPU_UNKNOWN;
	}
}

void TCCore::loadIntelLib()
{
	if (m_intelLoader.isLoaded()) return;

	if (!m_intelLoader.load("inteltemp.dll", {
		{"inteltemp_initialize", (void**)&m_inteltempInit},
		{"inteltemp_read_temperature", (void**)&m_inteltempReadTemp},
		{"inteltemp_get_last_error", (void**)&m_inteltempGetLastError},
		{"inteltemp_cleanup", (void**)&m_inteltempCleanup}
	}))
	{
		emit logMessage(QString("IntelTemp库%1: %2")
			.arg(m_intelLoader.state() == NativeLibraryLoader::State::LoadFailed ? "加载失败" : "函数解析失败")
			.arg(m_intelLoader.errorString()),
			LogManagement::LOG_ERROR);
		return;
	}

	// 初始化 IntelTemp（传递 IntelMSR.bin 完整路径）
	if (m_inteltempInit)
	{
		QString binPath = QCoreApplication::applicationDirPath() + "/IntelMSR.bin";
		int result = m_inteltempInit(reinterpret_cast<const wchar_t*>(binPath.utf16()));
		if (result == 0) // INTELLTEMP_SUCCESS
		{
			m_inteltempInitialized = true;
			emit logMessage(QString("IntelTemp初始化成功"), LogManagement::LogLevel::LOG_INFO);
		}
		else
		{
			char errorDetail[256] = {};
			if (m_inteltempGetLastError)
				m_inteltempGetLastError(errorDetail, sizeof(errorDetail));
			emit logMessage(QString("IntelTemp初始化失败(代码%1): %2")
				.arg(result)
				.arg(errorDetail[0] ? errorDetail : "未知错误"),
				LogManagement::LOG_ERROR);
		}
	}
}

void TCCore::loadAMDLib()
{
	if (m_amdLoader.isLoaded()) return;

	if (!m_amdLoader.load("amd_dll.dll", {
		{"amd_init", (void**)&m_amdInit},
		{"get_amd_temperature", (void**)&m_amdGetTemp}
	}))
	{
		emit logMessage(QString("AMD信息库加载失败!"), LogManagement::LogLevel::LOG_ERROR);
		return;
	}

	// 首次加载时调用 Init
	if (m_amdInit && !m_amdInitialized)
	{
		m_amdInit();
		m_amdInitialized = true;
	}
}

void TCCore::loadNvLib()
{
	if (m_nvLoader.isLoaded()) return;

	if (!m_nvLoader.load("nv_dll.dll", {
		{"nv_init", (void**)&m_nvInit},
		{"nv_get_temperature", (void**)&m_nvGetTemp}
	}))
	{
		emit logMessage(QString("NVIDIA信息库加载失败!"), LogManagement::LogLevel::LOG_ERROR);
		return;
	}

	// 首次加载时调用 Init
	if (m_nvInit && !m_nvInitialized)
	{
		m_nvInit();
		m_nvInitialized = true;
	}
}

void TCCore::loadCpuDllMgmt()
{
	if (m_cpuDllLoaded) return;
	loadCPUIDLib();
	if (m_cpuType == CPU_INTEL) {
		loadIntelLib();
	}
	else if (m_cpuType == CPU_AMD) {
		loadAMDLib();
	}
	else {
		emit logMessage(QString("CPU类型未知"), LogManagement::LogLevel::LOG_ERROR);
	}
	m_cpuDllLoaded = true;
}

void TCCore::loadGpuDllMgmt()
{
	if (m_gpuDllLoaded) return;
	loadNvLib();
	m_gpuDllLoaded = true;
}

int TCCore::getCpuTempMgmt()
{
	if (!m_cpuDllLoaded) {
		loadCpuDllMgmt();
	}

	switch (m_cpuType) {
	case CPU_INTEL:
		if (m_inteltempInitialized && m_inteltempReadTemp)
		{
			float temp = 0.0f;
			int result = m_inteltempReadTemp(TEMP_TYPE_CORE, &temp);
			if (result == 0) // INTELLTEMP_SUCCESS
				return static_cast<int>(temp);
			else
			{
				char errorDetail[256] = {};
				if (m_inteltempGetLastError)
					m_inteltempGetLastError(errorDetail, sizeof(errorDetail));
				emit logMessage(QString("IntelTemp读取失败(代码%1): %2")
					.arg(result)
					.arg(errorDetail[0] ? errorDetail : ""),
					LogManagement::LOG_WARNING);
				return -99;
			}
		}
		emit logMessage(QString("IntelTemp未初始化或函数指针无效"), LogManagement::LOG_WARNING);
		return -99;
	case CPU_AMD:
		return m_amdGetTemp ? m_amdGetTemp() : -99;
	default:
		emit logMessage(QString("错误，不支持的CPU类型"), LogManagement::LogLevel::LOG_ERROR);
		return -100;
	}
}

int TCCore::getGpuTempMgmt()
{
	if (!m_gpuDllLoaded) {
		loadGpuDllMgmt();
	}
	return m_nvGetTemp ? m_nvGetTemp() : -99;
}

void TCCore::initConfigFile()
{
	if (!m_config->isInitialized("TC"))
	{
		m_config->initSection("TC", "false");
		m_config->write("TC", "DataTransmissionDelay", "5000");
		m_config->initSection("TC", "true");
	}
	return;
}

int TCCore::getDataTrDelay()
{
	return m_config->read("TC", "DataTransmissionDelay").toInt();
}