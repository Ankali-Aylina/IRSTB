#pragma once
//#pragma execution_character_set("utf-8")

#include <QtWidgets/QMainWindow>
#include <QWidget>
#include <QApplication>
#include <QMouseEvent>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QThread>
#include "ui_TCV3.h"
#include <QPushButton>
#include <QAbstractButton>
#include <QUrl>

#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

#include "ExitWidgets.h"
#include "TCCore.h"
#include "BLEThread.h"
#include "IniManagement.h"
#include "LogManagement.h"
#include "UpdateLogDialog.h"
#include "AppModuleManager.h"
#include "IConfigProvider.h"
#include "BluetoothStatusPresenter.h"

class TCV3 : public QMainWindow
{
	Q_OBJECT

public:
	TCV3(QWidget* parent = nullptr);
	~TCV3();

	///窗口拖动
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;

private:

	QMetaObject::Connection m_fanControlConnection;

	// 蓝牙状态呈现器（替代 7 个 bleXxx() 方法和 3 个 BLE 图标）
	BluetoothStatusPresenter* m_blePresenter;

	QIcon m_cpuNormalIcon;
	QIcon m_cpuWarningIcon;
	QIcon m_gpuNormalIcon;
	QIcon m_gpuWarningIcon;

	QIcon m_buttonOnIcon;
	QIcon m_buttonOffIcon;

	void preloadIcons();

	///更新页面

	/// <summary>
	/// 更新按钮图标
	/// </summary>
	struct ButtonIcons {
		QIcon active; // 激活状态图标
		QIcon inactive; // 非激活状态图标
	};

	QVector<ButtonIcons> m_buttonIcons;  // 图标缓存
	QVector<QPushButton*> m_buttons;     // 菜单栏按钮集合
	QVector<QPushButton*> m_fanModeButtons; // 风扇模式按钮集合 {Auto, Silent, Performance}

	/// <summary>统一风扇模式 UI 更新和信号管理</summary>
	void setFanMode(int activeIdx, bool enableAutoConnection);

	///按钮图标更新
	void updateButtonIcon();

	/// <summary>
	/// 页面切换动画（淡入淡出）
	/// </summary>
	void animatePageSwitch(int targetIndex);

	QGraphicsOpacityEffect* m_pageOpacityEffect = nullptr;

	///窗口最大化与还原
	void showMaximizedOrNormal();

	/// 退出提示窗口
	void showExitWidgets();

	Ui::TCV3Class ui;

	//退出提示窗口类
	ExitWidgets* m_exitWidgets;

	//更新日志窗口类
	UpdateLogDialog* m_updateLogDialog;

	bool m_leftMousePressed;

	QPointF m_StartPoint;

	//最小化到托盘类
	QSystemTrayIcon* trayIcon;

	// 模块管理器（统一管理 TCCore、BLEThread 等子系统）
	AppModuleManager* m_moduleManager;

	// 配置提供者（依赖注入，替代全局单例）
	IConfigProvider* m_config;

	// 温度警告阈值（从配置加载，与 TemperatureConfig 保持同步）
	int m_warningCpuThreshold = 80;
	int m_warningGpuThreshold = 75;

	static constexpr int kMinDelay = 1;
	static constexpr int kMaxDelay = 10;
	static constexpr int kDefaultDelay = 5;

	/// <summary>
	/// 初始化配置文件
	/// </summary>
	void initConfigFile();

	/// <summary>
	/// CPU温度更新
	/// </summary>
	/// <param name="temperature"> CPU温度 </param>
	void onCpuTemperatureUpdated(int temperature);
	void onGpuTemperatureUpdated(int temperature);
	void updateTempDisplay(int temperature, int warningThreshold,
		QAbstractButton* iconBtn, QLabel* tempLabel, QProgressBar* bar,
		const QIcon& normalIcon, const QIcon& warningIcon);

	/// <summary>
	/// 智能模式
	/// </summary>
	void autoMode();

	/// <summary>
	/// 静音模式
	/// </summary>
	void silentMode();

	/// <summary>
	/// 全速模式
	/// </summary>
	void performanceMode();

	/// <summary>
	/// 重置设置
	/// </summary>
	void resetSettings();

	/// <summary>
	/// 自动启动
	/// </summary>
	void autoStart();

	/// <summary>
	/// 自动启动UI更新
	/// </summary>
	void autoStartUiUpdata();

	/// <summary>
	/// 设置自启动
	/// </summary>
	/// <param name="enable"></param>
	void setAutoStart(bool enable);

	/// <summary>
	/// 判断是否自启动
	/// </summary>
	/// <returns></returns>
	bool isAutoStartEnabled();

	/// <summary>
	/// 设置延时
	/// </summary>
	void dataDelay();

	/// <summary>
	/// 设置数据刷新延时
	/// </summary>
	void setdataDelay();

	/// <summary>
	/// 刷新延时显示UI
	/// </summary>
	/// <param name="data"></param>
	void dataDelayUiUpdate(int data);

	/// <summary>
	/// 延时增加
	/// </summary>
	void addDelay();

	/// <summary>
	/// 延时减少
	/// </summary>
	void minusDelay();

signals:
	/// <summary>
	/// 日志信号
	/// </summary>
	/// <param name="message"> 日志信息 </param>
	/// <param name="level"> 日志等级 </param>
	void logMessage(const QString& message, LogManagement::LogLevel level);

	/// <summary>
	/// 设置风扇静音模式
	/// </summary>
	/// <param name="buff"></param>
	void setFanAutoMode();

	/// <summary>
	/// 设置风扇静音模式
	/// </summary>
	/// <param name="buff"></param>
	void setFanSilentMode();

	/// <summary>
	/// 设置风扇静音模式
	/// </summary>
	/// <param name="buff"></param>
	void setFanPerformanceMode();

	/// <summary>
	/// 设置延时标志位
	/// </summary>
	/// <param name="data"></param>
	void setDataTrDelayUpdataFlag(bool flag);
};
