#pragma once

#include <cstdint>
#include <vector>
#include "IConfigProvider.h"

/// <summary>温度等级计算配置（可从 INI 加载）</summary>
struct TemperatureConfig
{
	int initCpuTemp = 40;
	int initGpuTemp = 40;
	int cpuStep = 10;
	int gpuStep = 8;
	int warningCpuThreshold = 80;
	int warningGpuThreshold = 75;
	int maxLevels = 5; // 温度等级数量 (0 ~ maxLevels-1)
	int historySize = 5;
	std::vector<float> weights = { 1.0f, 1.0f, 2.0f, 3.0f, 2.0f };

	/// <summary>从配置提供者加载，缺失项使用默认值</summary>
	static TemperatureConfig loadFromConfig(IConfigProvider* config)
	{
		TemperatureConfig cfg;
		if (!config) return cfg;

		auto readInt = [&](const char* key, int& target) {
			QVariant v = config->read("TC", key);
			if (v.isValid()) target = v.toInt();
		};
		readInt("InitCpuTemp", cfg.initCpuTemp);
		readInt("InitGpuTemp", cfg.initGpuTemp);
		readInt("CpuStep", cfg.cpuStep);
		readInt("GpuStep", cfg.gpuStep);
		readInt("WarningCpu", cfg.warningCpuThreshold);
		readInt("WarningGpu", cfg.warningGpuThreshold);
		readInt("HistorySize", cfg.historySize);

		// 范围校验：确保非法值不会导致异常行为
		if (cfg.initCpuTemp < 20 || cfg.initCpuTemp > 100) cfg.initCpuTemp = 40;
		if (cfg.initGpuTemp < 20 || cfg.initGpuTemp > 100) cfg.initGpuTemp = 40;
		if (cfg.cpuStep < 1 || cfg.cpuStep > 50) cfg.cpuStep = 10;
		if (cfg.gpuStep < 1 || cfg.gpuStep > 50) cfg.gpuStep = 8;
		if (cfg.warningCpuThreshold < 30 || cfg.warningCpuThreshold > 120) cfg.warningCpuThreshold = 80;
		if (cfg.warningGpuThreshold < 30 || cfg.warningGpuThreshold > 120) cfg.warningGpuThreshold = 75;
		if (cfg.historySize < 1 || cfg.historySize > 20) cfg.historySize = 5;
		if (cfg.weights.size() != static_cast<size_t>(cfg.historySize))
		{
			cfg.weights.assign(cfg.historySize, 1.0f);
		}
		return cfg;
	}
};

/// <summary>风扇模式枚举（替代硬编码 int 值）</summary>
enum class FanMode : uint8_t
{
	Auto = 0,
	Silent = 5,
	Performance = 6
};

/// <summary>将 FanMode 转换为发送值</summary>
inline uint8_t fanModeValue(FanMode mode) { return static_cast<uint8_t>(mode); }
