#include "ResourceExtractor.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QThread>
#include <QDebug>
#include <windows.h>
#include <wintrust.h>
#include <softpub.h>
#pragma comment(lib, "wintrust.lib")

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

// 前置声明：实现位于文件末尾（extract() 中调用）
static bool verifyExtractedFiles();

QString ResourceExtractor::extract()
{
    // 使用 app 名称 + 版本构建唯一的资源目录。
    // 注意：必须用 %LOCALAPPDATA%/<AppName> 而非 %TEMP%——%TEMP% 是共享目录，
    // 低权限进程可预占位同名 DLL/exe，配合 SetDllDirectoryW 可形成 DLL 劫持/本地提权
    QString appName = QCoreApplication::applicationName();
    QString subDir = appName + "_Resources";
    QString tempRoot = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
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
                // 快速路径：文件齐全，但仍需校验签名——防止目录被预占位/替换
                if (!verifyExtractedFiles()) {
                    return {};
                }
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

    // 对提取出的可执行文件做 Authenticode 校验（防止提取目录被恶意替换）
    if (!verifyExtractedFiles()) {
        return {};
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

// ============================================================================
// Authenticode 签名校验（防 DLL 劫持 / 提权执行被替换的安装包）
// ============================================================================

enum class SignatureStatus { Valid, NotSigned, Invalid };

/// <summary>校验文件的 Authenticode 签名（离线、不弹 UI）</summary>
static SignatureStatus verifyAuthenticode(const QString& filePath)
{
    WINTRUST_FILE_INFO fileInfo{};
    fileInfo.cbStruct = sizeof(fileInfo);
    fileInfo.pcwszFilePath = reinterpret_cast<LPCWSTR>(filePath.utf16());

    WINTRUST_DATA wintrustData{};
    wintrustData.cbStruct = sizeof(wintrustData);
    wintrustData.dwUIChoice = WTD_UI_NONE;
    wintrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    wintrustData.dwUnionChoice = WTD_CHOICE_FILE;
    wintrustData.pFile = &fileInfo;
    wintrustData.dwStateAction = WTD_STATEACTION_VERIFY;
    // 离线校验：不访问网络（避免启动卡顿），也不做吊销检查
    wintrustData.dwProvFlags = WTD_CACHE_ONLY_URL_RETRIEVAL | WTD_REVOCATION_CHECK_NONE;

    GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    LONG result = WinVerifyTrust(nullptr, &action, &wintrustData);

    // 无论结果如何都要关闭状态，释放内部句柄
    wintrustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(nullptr, &action, &wintrustData);

    if (result == ERROR_SUCCESS)
        return SignatureStatus::Valid;
    if (result == TRUST_E_NOSIGNATURE || result == TRUST_E_SUBJECT_FORM_UNKNOWN
        || result == TRUST_E_PROVIDER_UNKNOWN)
        return SignatureStatus::NotSigned;
    return SignatureStatus::Invalid;
}

/// <summary>校验所有提取出的 .dll/.exe/.sys；签名无效的文件会被删除并返回 false</summary>
static bool verifyExtractedFiles()
{
    bool allValid = true;
    for (const auto& entry : kEntries)
    {
        const QString lower = QString::fromLatin1(entry.diskName).toLower();
        if (!(lower.endsWith(".dll") || lower.endsWith(".exe") || lower.endsWith(".sys")))
            continue;

        const QString destPath = s_extractDir + "/" + entry.diskName;
        switch (verifyAuthenticode(destPath))
        {
        case SignatureStatus::Valid:
            break;
        case SignatureStatus::NotSigned:
            // 自编译 DLL 可能未签名：仅警告、不阻断（保持向后兼容）
            qWarning() << "ResourceExtractor: file not signed (verification skipped):"
                       << entry.diskName;
            break;
        case SignatureStatus::Invalid:
            qWarning() << "ResourceExtractor: signature INVALID, removing file:"
                       << entry.diskName;
            QFile::remove(destPath);
            allValid = false;
            break;
        }
    }
    return allValid;
}
