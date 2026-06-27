#include "ResourceExtractor.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <QDebug>
#include <windows.h>

// 将 QRC 路径映射到磁盘文件名
struct ResourceEntry {
    const char* qrcPath;     // QRC 内路径，如 ":/TCV3/res/lib/Cpu_Dll.dll"
    const char* diskName;    // 磁盘文件名，如 "Cpu_Dll.dll"
};

static const ResourceEntry kEntries[] = {
    // DLL 文件（NativeLibraryLoader 从文件系统加载）
    {":/TCV3/res/lib/amd_dll.dll",          "amd_dll.dll"},
    {":/TCV3/res/lib/Cpu_Dll.dll",          "Cpu_Dll.dll"},
    {":/TCV3/res/lib/nv_dll.dll",           "nv_dll.dll"},
    {":/TCV3/res/lib/inteltemp.dll",        "inteltemp.dll"},
    {":/TCV3/res/lib/WinRT_BLE_DLL.dll",   "WinRT_BLE_DLL.dll"},
    // 数据 / 工具文件（通过绝对路径访问）
    {":/TCV3/res/lib/IntelMSR.bin",         "IntelMSR.bin"},
    {":/TCV3/res/lib/PawnIO_setup.exe",     "PawnIO_setup.exe"},
    {":/TCV3/res/updatalog.md",             "updatalog.md"},
    // AMD Ryzen Master 驱动文件（amd_dll.dll 内部依赖）
    {":/TCV3/res/lib/bin/AMDRyzenMasterDriver.cat", "bin/AMDRyzenMasterDriver.cat"},
    {":/TCV3/res/lib/bin/AMDRyzenMasterDriver.inf", "bin/AMDRyzenMasterDriver.inf"},
    {":/TCV3/res/lib/bin/AMDRyzenMasterDriver.sys", "bin/AMDRyzenMasterDriver.sys"},
    {":/TCV3/res/lib/bin/Device.dll",              "bin/Device.dll"},
    {":/TCV3/res/lib/bin/Platform.dll",            "bin/Platform.dll"},
};

static QString s_extractDir;

QString ResourceExtractor::extract()
{
    // 使用 app 名称 + 版本构建唯一的临时目录
    QString appName = QCoreApplication::applicationName();
    QString subDir = appName + "_Resources";
    QString tempRoot = QDir::tempPath();
    s_extractDir = tempRoot + "/" + subDir;

    // 版本标记文件——内容为当前应用版本，版本变化时重新提取
    QString versionMarker = s_extractDir + "/.version";
    QString currentVersion = QCoreApplication::applicationVersion();

    QFile markerFile(versionMarker);
    if (markerFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString cachedVersion = QString::fromUtf8(markerFile.readAll()).trimmed();
        if (cachedVersion == currentVersion) {
            // 版本匹配，还需验证所有预期文件是否存在（防止新增文件未提取）
            bool allExist = true;
            for (const auto& entry : kEntries) {
                QFileInfo destInfo(s_extractDir + "/" + entry.diskName);
                if (!destInfo.exists()) {
                    qDebug() << "ResourceExtractor: missing file" << entry.diskName << ", re-extracting...";
                    allExist = false;
                    break;
                }
            }
            if (allExist) {
                SetDllDirectoryW(reinterpret_cast<LPCWSTR>(s_extractDir.utf16()));
                return s_extractDir;
            }
            // 有文件缺失，继续执行提取（不清空目录，走增量提取分支）
        }
        markerFile.close();
    }

    // 创建或清空目标目录
    QDir extractDir(s_extractDir);
    if (!extractDir.exists()) {
        if (!QDir().mkpath(s_extractDir)) {
            return {};
        }
    }

    // 提取所有资源文件
    for (const auto& entry : kEntries) {
        QString destPath = s_extractDir + "/" + entry.diskName;

        // 跳过已存在且大小一致的文件（增量提取）
        QFileInfo destInfo(destPath);
        QFileInfo qrcInfo(entry.qrcPath);
        // QRC 文件大小可能在某些环境下返回 0，此时强制重新提取
        if (destInfo.exists() && qrcInfo.size() > 0 && destInfo.size() == qrcInfo.size()) {
            continue;
        }

        // 确保目标子目录存在
        QFileInfo fi(destPath);
        QDir().mkpath(fi.absolutePath());

        // 从 QRC 复制到磁盘（含重试，避免旧进程 DLL 卸载未完成导致的文件锁冲突）
        bool copied = false;
        for (int retry = 0; retry < 5; ++retry) {
            // 先尝试删除旧文件（可能在版本更新时被旧进程锁定）
            if (destInfo.exists()) {
                QFile::remove(destPath);
            }
            if (QFile::copy(entry.qrcPath, destPath)) {
                copied = true;
                break;
            }
            qDebug() << "ResourceExtractor: retry" << retry << "for" << entry.diskName;
            QThread::msleep(100);
        }
        if (!copied) {
            qWarning() << "ResourceExtractor: failed to extract" << entry.qrcPath;
            return {};
        }

        // 移除只读属性（QRC 内文件可能带只读标记）
        SetFileAttributesW(reinterpret_cast<LPCWSTR>(destPath.utf16()),
                          FILE_ATTRIBUTE_NORMAL);
    }

    // 写入版本标记
    if (markerFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        markerFile.write(currentVersion.toUtf8());
        markerFile.close();
    }

    // 将提取目录加入 DLL 搜索路径
    SetDllDirectoryW(reinterpret_cast<LPCWSTR>(s_extractDir.utf16()));

    return s_extractDir;
}

QString ResourceExtractor::filePath(const QString& relativePath)
{
    if (s_extractDir.isEmpty()) return {};
    return s_extractDir + "/" + relativePath;
}
