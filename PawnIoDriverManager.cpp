#include "PawnIoDriverManager.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <windows.h>
#include <winreg.h>


bool PawnIoDriverManager::isInstalled() {
  return !installedVersion().isEmpty();
}

QString PawnIoDriverManager::installedVersion() {
  // 尝试读取注册表
  HKEY hKey;
  LSTATUS status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, kRegPath, 0,
                                 KEY_READ | KEY_WOW64_64KEY, &hKey);
  if (status != ERROR_SUCCESS) {
    // 回退 32 位视图
    status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, kRegPath, 0,
                           KEY_READ | KEY_WOW64_32KEY, &hKey);
  }

  if (status != ERROR_SUCCESS)
    return {};

  wchar_t buffer[64] = {};
  DWORD size = sizeof(buffer);
  status = RegQueryValueExW(hKey, L"DisplayVersion", nullptr, nullptr,
                            reinterpret_cast<LPBYTE>(buffer), &size);
  RegCloseKey(hKey);

  if (status != ERROR_SUCCESS)
    return {};

  return QString::fromWCharArray(buffer);
}

bool PawnIoDriverManager::isVersionValid() {
  QString ver = installedVersion();
  if (ver.isEmpty())
    return false;

  // 解析主版本号
  int major = ver.section('.', 0, 0).toInt();
  return major >= 2; // 需要 2.0+
}

bool PawnIoDriverManager::install(const QString &setupExePath) {
  if (!QFileInfo::exists(setupExePath))
    return false;

  QProcess process;
  process.start(setupExePath, {QStringLiteral("-install")});
  if (!process.waitForStarted(5000))
    return false;

  process.waitForFinished(30000); // 安装最多等待 30 秒
  return process.exitCode() == 0;
}
