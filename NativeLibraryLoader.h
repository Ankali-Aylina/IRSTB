#pragma once

#include <QLibrary>
#include <QString>
#include <vector>
#include <utility>

/// <summary>
/// 原生 DLL 库加载器——将 QLibrary 的加载、函数解析、状态管理统一封装。
/// 消除 TCCore 和 BLEThread 中重复的 DLL 加载模板代码（~200 行 → ~20 行/处）。
/// </summary>
class NativeLibraryLoader
{
public:
	enum class State { NotLoaded, Loaded, LoadFailed, ResolveFailed };

	NativeLibraryLoader() = default;

	/// <summary>加载 DLL 并批量解析函数</summary>
	/// <param name="dllPath">DLL 文件名（如 "Cpu_Dll.dll"）</param>
	/// <param name="functions">(函数名, 函数指针存储地址) 列表</param>
	/// <returns>是否全部成功</returns>
	bool load(const QString& dllPath,
	          const std::vector<std::pair<const char*, void**>>& functions);

	/// <summary>卸载 DLL 并重置状态</summary>
	void unload();

	State state() const { return m_state; }
	bool isLoaded() const { return m_state == State::Loaded; }
	QString errorString() const { return m_errorString; }

private:
	QLibrary m_library;
	State m_state = State::NotLoaded;
	QString m_errorString;
};
