#pragma once

#include <memory>

class QLockFile;

/// <summary>
/// 应用程序启动引导器——负责 UAC 提权、单实例检测、窗口创建等启动流程。
/// 将 main.cpp 从 ~77 行缩减到 ~10 行。
/// </summary>
class ApplicationBootstrap {
public:
  /// <summary>执行完整的启动流程</summary>
  /// <returns>应用程序退出码，-1 表示启动失败</returns>
  static int run(int argc, char *argv[]);

  /// <summary>释放单实例锁（在重启前调用，避免竞态条件）</summary>
  static void releaseLock();

private:
  static bool ensureAdminPrivilege(int argc, char *argv[]);
  static std::unique_ptr<QLockFile> s_lockFile;
};
