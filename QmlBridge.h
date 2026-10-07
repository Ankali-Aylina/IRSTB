#pragma once

#include <QCoreApplication>
#include <QObject>
#include <QMetaObject>
#include <QString>
#include <QtGui/qwindowdefs.h>   // WId（Windows 上为 HWND）
#include "LogManagement.h"
#include "BLEThread.h"           // 关机散热协议常量（kMin/Max/DefaultShutdownMinutes）

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
	// 关机散热时长（分钟）：持久化到 [TC]/ShutdownMinutes
	Q_PROPERTY(int shutdownMinutes READ shutdownMinutes NOTIFY shutdownMinutesChanged)
	Q_PROPERTY(int minShutdownMinutes READ minShutdownMinutes CONSTANT)
	Q_PROPERTY(int maxShutdownMinutes READ maxShutdownMinutes CONSTANT)
	Q_PROPERTY(int defaultShutdownMinutes READ defaultShutdownMinutes CONSTANT)
	Q_PROPERTY(bool autoStartEnabled READ autoStartEnabled NOTIFY autoStartEnabledChanged)
	// 关机散热结束后的提示语（空字符串=无提示）：提醒用户需重新选择风扇模式
	Q_PROPERTY(QString shutdownNotice READ shutdownNotice NOTIFY shutdownNoticeChanged)
	// BLE 状态：0=断开 1=搜索中 2=连接中 3=已连接 4=失败
	Q_PROPERTY(int bleState READ bleState NOTIFY bleStateChanged)
	Q_PROPERTY(QString bleStatusText READ bleStatusText NOTIFY bleStateChanged)
	Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
	// 主题模式：0=跟随系统 1=浅色 2=深色（持久化到 [App]/ThemeMode）
	Q_PROPERTY(int themeMode READ themeMode NOTIFY themeModeChanged)
	// 背景材质：0=无 1=Mica 2=Acrylic（持久化到 [App]/BackdropType）
	Q_PROPERTY(int backdropType READ backdropType NOTIFY backdropTypeChanged)
	// 系统通知（右下角弹出的原生通知）：持久化到 [App]/ToastNotify
	Q_PROPERTY(bool notifyEnabled READ notifyEnabled NOTIFY notifyEnabledChanged)
	// 下位机固件版本（如 "1.3.0"；空=未知/旧固件不支持版本查询）
	Q_PROPERTY(QString firmwareVersion READ firmwareVersion NOTIFY firmwareVersionChanged)
	// 下位机协议版本（如 "1.1"）
	Q_PROPERTY(QString firmwareProtocol READ firmwareProtocol NOTIFY firmwareProtocolChanged)
	// 固件是否支持关机散热（低于 1.2.0 会忽略 T 指令，界面据此禁用按钮）
	Q_PROPERTY(bool firmwareSupportsShutdown READ firmwareSupportsShutdown NOTIFY firmwareSupportsShutdownChanged)
	// 固件升级：进度 0~100；状态文本；进行中标记
	Q_PROPERTY(int otaPercent READ otaPercent NOTIFY otaProgressChanged)
	Q_PROPERTY(QString otaStatusText READ otaStatusText NOTIFY otaStatusTextChanged)
	Q_PROPERTY(bool otaRunning READ otaRunning NOTIFY otaRunningChanged)
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
	int shutdownMinutes() const { return m_shutdownMinutes; }
	int minShutdownMinutes() const { return kMinShutdownMinutes; }
	int maxShutdownMinutes() const { return kMaxShutdownMinutes; }
	int defaultShutdownMinutes() const { return kDefaultShutdownMinutes; }
	bool autoStartEnabled() const { return m_autoStartEnabled; }
	QString shutdownNotice() const { return m_shutdownNotice; }
	int bleState() const { return m_bleState; }
	QString bleStatusText() const { return m_bleStatusText; }
	QString appVersion() const { return QCoreApplication::applicationVersion(); }
	int themeMode() const { return m_themeMode; }
	int backdropType() const { return m_backdropType; }
	bool notifyEnabled() const { return m_notifyEnabled; }
	QString firmwareVersion() const { return m_firmwareVersion; }
	QString firmwareProtocol() const { return m_firmwareProtocol; }
	bool firmwareSupportsShutdown() const { return m_firmwareSupportsShutdown; }
	int otaPercent() const { return m_otaPercent; }
	QString otaStatusText() const { return m_otaStatusText; }
	bool otaRunning() const { return m_otaRunning; }
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

	/// <summary>关机散热时长 +/-（立即写入配置并在下次下发指令时生效）</summary>
	void changeShutdownMinutes(int delta);

	/// <summary>下发关机散热指令：风扇全速运行设定时长后自动关闭</summary>
	void startShutdownCooling();

	/// <summary>清除关机散热结束提示（仅信息型；用户重新选择模式时调用）</summary>
	void clearShutdownNotice();

	/// <summary>用户点提示条的关闭按钮：无条件清除</summary>
	void dismissShutdownNotice();

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

	/// <summary>切换系统通知开关（写入 [App]/ToastNotify）</summary>
	void toggleNotify();

	/// <summary>选择固件文件并开始升级</summary>
	void chooseFirmwareAndUpgrade();

	/// <summary>取消正在进行的固件升级</summary>
	void cancelFirmwareUpgrade();

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
	void shutdownMinutesChanged(int minutes);
	void autoStartEnabledChanged(bool enabled);
	void shutdownNoticeChanged();
	void bleStateChanged();
	void themeModeChanged(int mode);
	void backdropTypeChanged(int type);
	void notifyEnabledChanged(bool enabled);
	void firmwareVersionChanged(const QString& version);
	void firmwareProtocolChanged(const QString& protocol);
	void firmwareSupportsShutdownChanged(bool supported);
	void otaProgressChanged();
	void otaStatusTextChanged();
	void otaRunningChanged();

	/// <summary>转发给 BLEThread：开始/取消固件升级（QmlBridge 是唯一的中枢，
	/// 界面不直接接触业务模块）</summary>
	void requestFirmwareUpgrade(const QString& filePath);
	void requestFirmwareUpgradeCancel();
	void systemDarkChanged(bool dark);
	/// <summary>QML 收到后应显示主窗口（托盘"恢复"触发）</summary>
	void restoreRequested();
	void logMessage(const QString& message, LogManagement::LogLevel level);

private:
	void setupModules();
	void setupTrayIcon();
	void setupBleConnections();
	void setBleUiState(BleUiState state, const QString& text);

	/// <summary>显示提示条（覆盖前一条）。autoDismiss=true 时 6 秒后自动消失，
	/// false 表示必须由用户处理（如"蓝牙未连接"），不会被自动流程清掉</summary>
	void showShutdownNotice(const QString& text, bool autoDismiss);

	/// <summary>按开关设置弹出系统通知（窗口最小化/隐藏到托盘时同样可见）</summary>
	void showSystemNotification(const QString& title, const QString& message);
	bool isAutoStartEnabled();   // 读注册表（HKCU\...\Run）
	void setAutoStart(bool enable); // 写注册表
	bool detectSystemDark();      // 读注册表 AppsUseLightTheme（0=深色 1=浅色）
	void applyDwmBackdrop();       // 应用 Mica/Acrylic（Win11 22H2+）
	void applyDwmDark(bool dark);  // 应用 DWM 暗色模式

	static constexpr int kMinDelay = 1;
	static constexpr int kMaxDelay = 10;
	static constexpr int kDefaultDelay = 5;

	// 系统通知：标题与正文都由系统按通知宽度自动换行，因此标题要短、正文一句话
	static constexpr int kNotifyTimeoutMs = 10000;   // 通知停留时长
	static constexpr int kNotifyThrottleMs = 3000;   // 重复通知的最小间隔（防刷屏）

	AppModuleManager* m_moduleManager = nullptr;
	IConfigProvider* m_config = nullptr;
	QSystemTrayIcon* m_trayIcon = nullptr;
	QMetaObject::Connection m_fanControlConnection;

	int m_cpuTemp = 0;
	int m_gpuTemp = 0;
	int m_warningCpu = 80;
	int m_warningGpu = 75;
	int m_delayUiSeconds = kDefaultDelay;
	int m_shutdownMinutes = kDefaultShutdownMinutes;
	QString m_shutdownNotice;   // 散热结束后的提示（空=不显示）
	bool m_shutdownNoticeAuto = false;  // 当前提示是否为"可自动消失"的信息型
	QTimer* m_shutdownNoticeTimer = nullptr;
	qint64 m_lastNotifyMs = 0;          // 上一条系统通知的时间戳（节流用）
	bool m_autoStartEnabled = false;
	int m_bleState = BleDisconnected;
	QString m_bleStatusText = QStringLiteral("未连接");
	int m_themeMode = 0;
	int m_backdropType = 1;
	bool m_notifyEnabled = true;   // 系统通知默认开启
	// 下位机版本信息（连接后由 BLEThread 查询回传）
	QString m_firmwareVersion;
	QString m_firmwareProtocol;
	bool m_firmwareSupportsShutdown = true;   // 未知时按"支持"处理，避免误禁用功能
	// 固件升级界面状态
	int m_otaPercent = 0;
	QString m_otaStatusText;
	bool m_otaRunning = false;
	bool m_systemDark = false;
	WId m_hwnd{};   // 窗口句柄（WId 在 Windows 上可能是 HWND 或整型句柄，直接保存避免转换问题）
};
