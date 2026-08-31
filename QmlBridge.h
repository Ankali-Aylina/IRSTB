#pragma once

#include <QCoreApplication>
#include <QObject>
#include <QMetaObject>
#include <QString>
#include <QtGui/qwindowdefs.h>   // WId（Windows 上为 HWND）
#include "LogManagement.h"

class AppModuleManager;
class IConfigProvider;
class QSystemTrayIcon;

/// <summary>
/// C++ 与 QML 之间的桥接层（替代原 TCV3 Widgets 主窗口的 UI 职责）。
/// 负责：模块生命周期管理（AppModuleManager）、系统托盘、风扇模式/延时/自启动
/// 等 UI 意图到业务模块的转发，以及将模块状态（温度/BLE）暴露为 QML 属性。
/// 业务层（TCCore/BLEThread/配置/资源提取）保持原样，仅 UI 层迁移到 QML。
/// </summary>
class QmlBridge : public QObject
{
	Q_OBJECT

	// --- QML 可绑定属性 ---
	Q_PROPERTY(int cpuTemp READ cpuTemp NOTIFY cpuTempChanged)
	Q_PROPERTY(int gpuTemp READ gpuTemp NOTIFY gpuTempChanged)
	Q_PROPERTY(int warningCpu READ warningCpu CONSTANT)
	Q_PROPERTY(int warningGpu READ warningGpu CONSTANT)
	Q_PROPERTY(int delayUiSeconds READ delayUiSeconds NOTIFY delayUiSecondsChanged)
	Q_PROPERTY(bool autoStartEnabled READ autoStartEnabled NOTIFY autoStartEnabledChanged)
	// BLE 状态：0=断开 1=搜索中 2=连接中 3=已连接 4=失败
	Q_PROPERTY(int bleState READ bleState NOTIFY bleStateChanged)
	Q_PROPERTY(QString bleStatusText READ bleStatusText NOTIFY bleStateChanged)
	Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
	// 主题模式：0=跟随系统 1=浅色 2=深色（持久化到 [App]/ThemeMode）
	Q_PROPERTY(int themeMode READ themeMode NOTIFY themeModeChanged)
	// 背景材质：0=无 1=Mica 2=Acrylic（持久化到 [App]/BackdropType）
	Q_PROPERTY(int backdropType READ backdropType NOTIFY backdropTypeChanged)
	// 系统深浅色（读注册表 AppsUseLightTheme，比 Qt.styleHints.colorScheme 可靠）
	Q_PROPERTY(bool systemDark READ systemDark NOTIFY systemDarkChanged)

public:
	enum BleUiState {
		BleDisconnected = 0,
		BleScanning = 1,
		BleConnecting = 2,
		BleConnected = 3,
		BleFailed = 4,
	};
	Q_ENUM(BleUiState)

	explicit QmlBridge(QObject* parent = nullptr);
	~QmlBridge() override;

	int cpuTemp() const { return m_cpuTemp; }
	int gpuTemp() const { return m_gpuTemp; }
	int warningCpu() const { return m_warningCpu; }
	int warningGpu() const { return m_warningGpu; }
	int delayUiSeconds() const { return m_delayUiSeconds; }
	bool autoStartEnabled() const { return m_autoStartEnabled; }
	int bleState() const { return m_bleState; }
	QString bleStatusText() const { return m_bleStatusText; }
	QString appVersion() const { return QCoreApplication::applicationVersion(); }
	int themeMode() const { return m_themeMode; }
	int backdropType() const { return m_backdropType; }
	bool systemDark() const { return m_systemDark; }

public slots:
	/// <summary>风扇模式：0=自动 1=静音 2=全速</summary>
	void setFanMode(int mode);

	/// <summary>蓝牙重连</summary>
	void reconnectBle();

	/// <summary>延时 +/-（只更新 UI 显示值，需 applyDelay 生效）</summary>
	void changeDelay(int delta);

	/// <summary>将 UI 延时值写入配置并通知 TCCore 立即生效</summary>
	void applyDelay();

	/// <summary>切换开机自启</summary>
	void toggleAutoStart();

	/// <summary>重置设置（删除配置并重启）</summary>
	void resetSettings();

	/// <summary>打开支持页面</summary>
	void openSupportUrl();

	/// <summary>读取更新日志（QRC 内 markdown 原文）</summary>
	QString readUpdateLog();

	/// <summary>设置主题模式（0=跟随系统 1=浅色 2=深色）</summary>
	void setThemeMode(int mode);

	/// <summary>设置背景材质（0=无 1=Mica 2=Acrylic）</summary>
	void setBackdropType(int type);

	/// <summary>保存窗口句柄并应用当前材质（由启动流程在 QML 加载后调用）</summary>
	void applyWindowBackdrop(WId hwnd);

	/// <summary>应用 DWM 暗色模式（QML 在主题变化时调用）</summary>
	void applyDarkMode(bool dark);

	/// <summary>退出程序</summary>
	void quitApp();

	/// <summary>从托盘恢复窗口</summary>
	void restoreFromTray();

signals:
	void cpuTempChanged(int temperature);
	void gpuTempChanged(int temperature);
	void delayUiSecondsChanged(int seconds);
	void autoStartEnabledChanged(bool enabled);
	void bleStateChanged();
	void themeModeChanged(int mode);
	void backdropTypeChanged(int type);
	void systemDarkChanged(bool dark);
	/// <summary>QML 收到后应显示主窗口（托盘"恢复"触发）</summary>
	void restoreRequested();
	void logMessage(const QString& message, LogManagement::LogLevel level);

private:
	void setupModules();
	void setupTrayIcon();
	void setupBleConnections();
	void setBleUiState(BleUiState state, const QString& text);
	bool isAutoStartEnabled();   // 读注册表（HKCU\...\Run）
	void setAutoStart(bool enable); // 写注册表
	bool detectSystemDark();      // 读注册表 AppsUseLightTheme（0=深色 1=浅色）
	void applyDwmBackdrop();       // 应用 Mica/Acrylic（Win11 22H2+）
	void applyDwmDark(bool dark);  // 应用 DWM 暗色模式

	static constexpr int kMinDelay = 1;
	static constexpr int kMaxDelay = 10;
	static constexpr int kDefaultDelay = 5;

	AppModuleManager* m_moduleManager = nullptr;
	IConfigProvider* m_config = nullptr;
	QSystemTrayIcon* m_trayIcon = nullptr;
	QMetaObject::Connection m_fanControlConnection;

	int m_cpuTemp = 0;
	int m_gpuTemp = 0;
	int m_warningCpu = 80;
	int m_warningGpu = 75;
	int m_delayUiSeconds = kDefaultDelay;
	bool m_autoStartEnabled = false;
	int m_bleState = BleDisconnected;
	QString m_bleStatusText = QStringLiteral("未连接");
	int m_themeMode = 0;
	int m_backdropType = 1;
	bool m_systemDark = false;
	WId m_hwnd{};   // 窗口句柄（WId 在 Windows 上可能是 HWND 或整型句柄，直接保存避免转换问题）
};
