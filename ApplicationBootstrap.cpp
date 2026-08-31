#include "ApplicationBootstrap.h"
#include "PawnIoDriverManager.h"
#include "QmlBridge.h"
#include "ResourceExtractor.h"
#include "resource.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QLockFile>
#include <QMessageBox>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QUrl>
#include <windows.h>
#include <shellapi.h>

bool ApplicationBootstrap::ensureAdminPrivilege(int argc, char *argv[]) {
  // 检查是否已以管理员权限运行：读取进程令牌的 TokenElevation 属性。
  // 不用 IsUserAnAdmin()（该 API 在部分 Windows SDK 中受版本宏/分区保护，会编译失败），
  // 也不依赖 PATH 中的 net.exe
  HANDLE token = nullptr;
  if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
    TOKEN_ELEVATION elevation{};
    DWORD size = sizeof(elevation);
    const BOOL got = GetTokenInformation(token, TokenElevation, &elevation, size, &size);
    CloseHandle(token);
    if (got && elevation.TokenIsElevated)
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

  // 关键：必须使用支持自定义的样式（Basic）。
  // Qt 6.7+ 在 Windows 11 上默认使用 "Windows 11" 样式，该原生样式
  // 拒绝自定义控件的 background/contentItem —— 会导致所有按钮自定义颜色
  // 失效并显示默认外观（浅色按钮/默认文字），此前"按钮看不清"的根因。
  QQuickStyle::setStyle(QStringLiteral("Basic"));

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

  // --- 加载 QML 界面（UI 层已从 Qt Widgets 迁移到 Qt Quick） ---
  QmlBridge bridge;
  QQmlApplicationEngine engine;
  QStringList qmlErrors;
  QObject::connect(&engine, &QQmlEngine::warnings, &engine,
                   [&qmlErrors](const QList<QQmlError>& warnings) {
                     for (const auto& w : warnings) {
                       qWarning().noquote() << "QML:" << w.toString();
                       qmlErrors << w.toString();
                     }
                   });
  engine.rootContext()->setContextProperty(QStringLiteral("bridge"), &bridge);
  engine.load(QUrl(QStringLiteral("qrc:/TCV3/qml/main.qml")));
  if (engine.rootObjects().isEmpty()) {
    const QString detail = qmlErrors.isEmpty()
        ? QStringLiteral("（无详细错误输出，通常是 QML 运行时插件缺失："
                         "请确保 exe 旁存在 qml/ 目录，或设置 QT_QML_IMPORT_PATH）")
        : qmlErrors.join(QLatin1Char('\n'));
    QMessageBox::warning(nullptr, QStringLiteral("启动失败"),
                         QStringLiteral("无法加载界面资源。\n\n") + detail);
    return -1;
  }

  // 应用 Windows 11 Mica/Acrylic 背景材质（QML 窗口保持透明以透出材质）
  if (auto* win = qobject_cast<QQuickWindow*>(engine.rootObjects().first())) {
    bridge.applyWindowBackdrop(win->winId());
  }

  int ret = app.exec();
  s_lockFile.reset();  // 正常退出时释放锁
  return ret;
}
