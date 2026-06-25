#pragma once

#include <QObject>
#include <QIcon>

class QLabel;
class QPushButton;
class QProgressBar;

/// <summary>
/// 蓝牙状态呈现器——负责将 BLE 状态变化映射为 UI 更新。
/// 将 TCV3 中的 6 个 bleXxx() 方法集中到此单一职责类中。
/// </summary>
class BluetoothStatusPresenter : public QObject
{
	Q_OBJECT

public:
	BluetoothStatusPresenter(QLabel* statusLabel, QPushButton* bleButton,
	                         QProgressBar* progressBar,
	                         const QIcon& onIcon, const QIcon& offIcon,
	                         const QIcon& errorIcon, QObject* parent = nullptr);

public slots:
	void onScanStarted();
	void onScanTimeout();
	void onConnecting();
	void onConnectionFailed();
	void onConnected();
	void onDisconnected();
	void onReconnect();

private:
	QLabel* m_statusLabel;
	QPushButton* m_bleButton;
	QProgressBar* m_progressBar;
	QIcon m_onIcon;
	QIcon m_offIcon;
	QIcon m_errorIcon;
};
