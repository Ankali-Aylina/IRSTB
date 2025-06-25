#pragma once
//#pragma execution_character_set("utf-8")

#include <QtWidgets/QMainWindow>
#include <QWidget>
#include <QApplication>
#include <QMouseEvent>
#include <QmessageBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QThread>
#include <QSystemSemaphore>
#include <QSharedMemory>
#include "ui_TCV3.h"
#include <QObject>
#include <QDesktopServices>
#include <QPushButton>
#include <QUrl>
#include <QLockFile>
#include <QProcess>

#include "ExitWidgets.h"
#include "TCCore.h"
#include "BLEThread.h"
#include "IniManagement.h"
#include "LogManagement.h"
#include "UpdateLogDialog.h"

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

	bool isAppRunning(const QString& lockName);

private:

	QMetaObject::Connection m_fanControlConnection;

	///更新页面
	// void updateIndex();

	/// <summary>
	/// 更新按钮图标
	/// </summary>
	struct ButtonIcons {
		QIcon active; // 激活状态图标
		QIcon inactive; // 非激活状态图标
	};

	QVector<ButtonIcons> m_buttonIcons;  // 图标缓存
	QVector<QPushButton*> m_buttons;     // 按钮集合

	///按钮图标更新
	void updateButtonIcon();

	///窗口最大化与还原
	void showMaximizedOrNormal();

	/// 退出提示窗口
	void showExitWidgets();

	void bleReconnectUi();

	/// <summary>
	/// 蓝牙搜索
	/// </summary>
	void bleScanStarting();

	/// <summary>
	/// 蓝牙搜索超时
	/// </summary>
	void bleScanTimeout();

	/// <summary>
	/// 蓝牙连接中
	/// </summary>
	void bleConnecting();

	/// <summary>
	/// 蓝牙连接失败
	/// </summary>
	void bleConnectionFailed();

	/// <summary>
	/// 蓝牙连接成功
	/// </summary>
	void bleConnected();

	void bleDisconnected();

	Ui::TCV3Class ui;

	//退出提示窗口类
	ExitWidgets* m_exitWidgets;

	//更新日志窗口类
	UpdateLogDialog* m_updateLogDialog;

	bool m_leftMousePressed;

	QPointF m_StartPoint;

	//最小化到托盘类
	QSystemTrayIcon* trayIcon;

	//log管理类
	LogManagement* o_Log;

	//TC核心功能类
	TCCore* o_TCCore;
	QThread* q_TCCore;

	//蓝牙线程
	BLEThread* o_BLEThread;
	QThread* q_BLEThread;

	/// <summary>
	/// 初始化配置文件
	/// </summary>
	void initConfigFile();

	/// <summary>
	/// CPU温度更新
	/// </summary>
	/// <param name="temperature"> CPU温度 </param>
	void onCpuTemperatureUpdated(int temperature);

	/// <summary>
	/// GPU温度更新
	/// </summary>
	/// <param name="temperature"> GPU温度 </param>
	void onGpuTemperatureUpdated(int temperature);

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
