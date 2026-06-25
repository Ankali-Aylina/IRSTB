#pragma once

#include <QString>

/// <summary>
/// PawnIO 内核驱动管理器——检测驱动安装状态、版本，并提供静默安装。
/// 启动时在 ApplicationBootstrap 中调用，确保驱动就绪后再初始化硬件。
/// </summary>
class PawnIoDriverManager {
public:
  /// <summary>检查驱动是否已安装（通过注册表）</summary>
  static bool isInstalled();

  /// <summary>获取已安装驱动的版本字符串，未安装返回空</summary>
  static QString installedVersion();

  /// <summary>静默安装/更新驱动</summary>
  /// <param name="setupExePath">PawnIO_setup.exe 的完整路径</param>
  /// <returns>安装是否成功</returns>
  static bool install(const QString &setupExePath);

  /// <summary>检查版本是否满足最低要求</summary>
  static bool isVersionValid();

private:
  static constexpr const wchar_t *kRegPath =
      L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\PawnIO";
};
