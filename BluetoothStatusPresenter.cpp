#include "BluetoothStatusPresenter.h"
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>

BluetoothStatusPresenter::BluetoothStatusPresenter(
	QLabel* statusLabel, QPushButton* bleButton, QProgressBar* progressBar,
	const QIcon& onIcon, const QIcon& offIcon, const QIcon& errorIcon,
	QObject* parent)
	: QObject(parent)
	, m_statusLabel(statusLabel)
	, m_bleButton(bleButton)
	, m_progressBar(progressBar)
	, m_onIcon(onIcon)
	, m_offIcon(offIcon)
	, m_errorIcon(errorIcon)
{
}

void BluetoothStatusPresenter::onScanStarted()
{
	m_bleButton->setIcon(m_offIcon);
	m_statusLabel->setText(QStringLiteral("搜索中..."));
	m_progressBar->setMaximum(0);
}

void BluetoothStatusPresenter::onScanTimeout()
{
	m_bleButton->setIcon(m_errorIcon);
	m_statusLabel->setText(QStringLiteral("搜索超时"));
	m_progressBar->setMaximum(100);
	m_progressBar->setValue(100);
}

void BluetoothStatusPresenter::onConnecting()
{
	m_statusLabel->setText(QStringLiteral("连接中..."));
}

void BluetoothStatusPresenter::onConnectionFailed()
{
	m_bleButton->setIcon(m_errorIcon);
	m_statusLabel->setText(QStringLiteral("连接失败"));
	m_statusLabel->setStyleSheet(QStringLiteral("color: #e04050;"));
	m_progressBar->setMaximum(100);
	m_progressBar->setValue(100);
}

void BluetoothStatusPresenter::onConnected()
{
	m_bleButton->setIcon(m_onIcon);
	m_statusLabel->setText(QStringLiteral("连接完成"));
	m_statusLabel->setStyleSheet(QStringLiteral("color: #e8e8e8"));
	m_progressBar->setMaximum(100);
	m_progressBar->setValue(0);
}

void BluetoothStatusPresenter::onDisconnected()
{
	m_bleButton->setIcon(m_errorIcon);
	m_statusLabel->setText(QStringLiteral("断开连接"));
	m_statusLabel->setStyleSheet(QStringLiteral("color: #e04050;"));
	m_progressBar->setMaximum(100);
	m_progressBar->setValue(100);
}

void BluetoothStatusPresenter::onReconnect()
{
	m_statusLabel->setText(QStringLiteral("重连中..."));
}
