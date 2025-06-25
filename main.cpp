#include "TCV3.h"
#include <QtWidgets/QApplication>
#include <QMessageBox>
#include <QProcess>
#include <QLockFile>
#include <windows.h>
#include <shellapi.h>

bool runAsAdmin(const QString& program, const QStringList& arguments) {
	SHELLEXECUTEINFO sei = { 0 };
	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.hwnd = NULL;
	sei.lpVerb = L"runas";
	std::wstring programWStr = program.toStdWString(); // 将临时对象转换为局部变量
	sei.lpFile = programWStr.c_str(); // 现在指向局部变量的内存
	std::wstring argumentsWStr = arguments.join(" ").toStdWString();
	sei.lpParameters = argumentsWStr.c_str(); // 现在指向局部变量的内存
	sei.lpDirectory = NULL;
	sei.nShow = SW_NORMAL;
	sei.hInstApp = NULL;

	if (!ShellExecuteEx(&sei)) {
		DWORD dwError = GetLastError();
		if (dwError == ERROR_CANCELLED) {
			// 用户取消UAC提示
			return false;
		}
		else {
			// 其他错误
			return false;
		}
	}

	// 检查 hProcess 是否为 NULL
	if (sei.hProcess == NULL) {
		return false; // 如果为 NULL，则返回错误
	}

	// 等待新进程结束
	WaitForSingleObject(sei.hProcess, INFINITE);
	DWORD dwExitCode;
	GetExitCodeProcess(sei.hProcess, &dwExitCode);
	CloseHandle(sei.hProcess);

	return true;
}

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);

	// 检查当前程序是否已经以管理员权限运行
	bool isAdmin = false;
	QProcess process;
	process.start("net", QStringList() << "session");
	if (process.waitForFinished()) {
		QString output(process.readAllStandardOutput());
		isAdmin = !output.isEmpty();
	}

	if (!isAdmin) {
		// 如果当前程序没有管理员权限，则尝试以管理员权限重新启动程序
		QString program = QApplication::applicationFilePath();
		QStringList arguments = QCoreApplication::arguments();
		bool success = runAsAdmin(program, arguments);
		if (!success) {
			// 无法以管理员权限启动程序
			return -1;
		}
	}
	else {
		// 以管理员权限运行的代码

		// 1. 定义锁文件路径（确保唯一性）
		QString lockName = QCoreApplication::applicationName() + ".lock"; // 替换为你的应用唯一标识
		QString lockPath = QDir::tempPath() + "/" + lockName;

		// 2. 创建锁文件对象
		QLockFile lockFile(lockPath);

		// 3. 尝试获取锁（超时时间100ms）
		if (!lockFile.tryLock(100)) {
			// 获取锁失败，说明已有实例在运行
			QMessageBox::warning(
				nullptr,
				"警告",
				"程序已在运行，无法重复启动！"
			);
			return -1;
		}

		TCV3 w;
		w.show();
		return a.exec();
	}
}