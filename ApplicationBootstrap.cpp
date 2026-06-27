#include "ApplicationBootstrap.h"
#include "PawnIoDriverManager.h"
#include "ResourceExtractor.h"
#include "resource.h"
#include "TCV3.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QLockFile>
#include <QMessageBox>
#include <QProcess>
#include <windows.h>
#include <shellapi.h>

bool ApplicationBootstrap::ensureAdminPrivilege(int argc, char *argv[]) {
  // 检查是否已以管理员权限运行
  QProcess process;
  process.start("net", QStringList() << "session");
  if (process.waitForFinished()) {
    QString output(process.readAllStandardOutput());
    if (!output.isEmpty())
      return true; // 已是管理员
  }

  // 尝试以管理员权限重新启动（原进程随即退出，由新进程接管）
  QString program = QApplication::applicationFilePath();
  QStringList arguments;
  for (int i = 1; i < argc; ++i)
    arguments << QString::fromLocal8Bit(argv[i]);

  SHELLEXECUTEINFOW sei = {sizeof(SHELLEXECUTEINFOW)};
  sei.fMask = SEE_MASK_NOCLOSEPROCESS;
  sei.lpVerb = L"runas";
  sei.lpFile = reinterpret_cast<const wchar_t *>(program.utf16());
  std::wstring argsWStr = arguments.join(' ').toStdWString();
  sei.lpParameters = argsWStr.c_str();
  sei.nShow = SW_NORMAL;

  if (!ShellExecuteExW(&sei))
    return false;

  // 提权成功，原进程退出（新进程会以管理员身份独立运行）
  if (sei.hProcess)
    CloseHandle(sei.hProcess);
  return false; // 返回 false 让 run() 退出，不再创建窗口
}

std::unique_ptr<QLockFile> ApplicationBootstrap::s_lockFile;

void ApplicationBootstrap::releaseLock() {
  if (s_lockFile) {
    s_lockFile->unlock();
    s_lockFile.reset();
  }
}

int ApplicationBootstrap::run(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // 设置应用版本（供 ResourceExtractor 版本缓存使用）
  app.setApplicationVersion(APP_VERSION_STR);

  if (!ensureAdminPrivilege(argc, argv))
    return -1;

  // 单实例锁（静态成员，可通过 releaseLock() 在重启前手动释放）
  QString lockPath =
      QDir::tempPath() + "/" + QCoreApplication::applicationName() + ".lock";
  s_lockFile = std::make_unique<QLockFile>(lockPath);
  if (!s_lockFile->tryLock(100)) {
    QMessageBox::warning(nullptr, "警告", "程序已在运行，无法重复启动！");
    return -1;
  }

  // --- 从 QRC 提取运行时资源到临时目录 ---
  QString extractDir = ResourceExtractor::extract();
  if (extractDir.isEmpty()) {
    QMessageBox::warning(nullptr, QStringLiteral("启动失败"),
                         QStringLiteral("无法解压运行时资源文件。"));
    return -1;
  }

  // --- PawnIO 驱动检测与安装 ---
  if (!PawnIoDriverManager::isVersionValid()) {
    QString msg =
        PawnIoDriverManager::isInstalled()
            ? QStringLiteral(
                  "PawnIO 驱动版本过旧，需要更新才能正常使用。\n是否立即更新？")
            : QStringLiteral("检测到 PawnIO 驱动未安装，这是 Intel CPU "
                             "温度监控所必需的。\n是否立即安装？");

    auto result =
        QMessageBox::question(nullptr, QStringLiteral("驱动安装"), msg,
                              QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
      QString setupPath = ResourceExtractor::filePath("PawnIO_setup.exe");
      if (!PawnIoDriverManager::install(setupPath)) {
        QMessageBox::warning(
            nullptr, QStringLiteral("安装失败"),
            QStringLiteral("PawnIO 驱动安装失败，温度监控功能将不可用。"));
      }
    }
  }

  TCV3 window;
  window.show();
  int ret = app.exec();
  s_lockFile.reset();  // 正常退出时释放锁
  return ret;
}
