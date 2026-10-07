#include "ResourceExtractor.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QStandardPaths>
#include <QTextStream>
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

/// <summary>资源清单路径：记录每个已提取文件的文件名与大小，作为"内容指纹"。
/// 只用应用版本号做缓存判据是不够的——同一个版本号下重新构建、替换了某个 DLL 时
/// 版本号不变，旧文件会被误判为"无需更新"（实际踩过：换了新版 BLE DLL 却没生效）</summary>
static QString manifestPath() { return s_extractDir + "/.manifest"; }

/// <summary>读取清单为 "文件名|大小" 的映射</summary>
static QHash<QString, qint64> readManifest()
{
    QHash<QString, qint64> map;
    QFile f(manifestPath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return map;

    while (!f.atEnd()) {
        const QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (line.isEmpty()) continue;
        const int sep = line.lastIndexOf(QLatin1Char('|'));
        if (sep <= 0) continue;
        bool ok = false;
        const qint64 size = line.mid(sep + 1).toLongLong(&ok);
        if (ok) map.insert(line.left(sep), size);
    }
    return map;
}

/// <summary>写入清单（提取/校验通过后调用）</summary>
static void writeManifest()
{
    QFile f(manifestPath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return;

    QTextStream out(&f);
    for (const auto& entry : kEntries) {
        QFileInfo fi(s_extractDir + "/" + entry.diskName);
        out << entry.diskName << '|' << fi.size() << '\n';
    }
}

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

    // 版本不匹配说明它是上一次发布留下的目录：必须整体重新提取。
    // 不能只依赖下面"大小一致就跳过"的增量判据——QRC 内文件是压缩存储的，
    // QFileInfo(qrcPath).size() 对它们返回的是压缩后大小，无法与磁盘上的
    // 解压文件比较；靠它判断会出现"版本已更新但旧 DLL 被保留"的问题
    bool versionChanged = true;

    QFile markerFile(versionMarker);
    if (markerFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString cachedVersion = QString::fromUtf8(markerFile.readAll()).trimmed();
        versionChanged = (cachedVersion != currentVersion);
        if (!versionChanged) {
            // 版本号相同：还要确认"每个文件都在，且大小与上次提取的清单一致"。
            // 清单缺失（例如旧版本程序留下的目录）按"需要重新提取"处理
            const QHash<QString, qint64> manifest = readManifest();
            bool allMatch = !manifest.isEmpty();
            for (const auto& entry : kEntries) {
                if (!allMatch) break;

                const QString destPath = s_extractDir + "/" + entry.diskName;
                QFileInfo destInfo(destPath);
                if (!destInfo.exists()) {
                    qDebug() << "ResourceExtractor: missing file" << entry.diskName << ", re-extracting...";
                    allMatch = false;
                    break;
                }
                const auto it = manifest.constFind(QString::fromLatin1(entry.diskName));
                if (it == manifest.constEnd() || it.value() != destInfo.size()) {
                    qDebug() << "ResourceExtractor: changed file" << entry.diskName << ", re-extracting...";
                    allMatch = false;
                    break;
                }
            }
            if (allMatch) {
                // 快速路径：文件齐全且内容未变，但仍需校验签名——防止目录被预占位/替换
                if (!verifyExtractedFiles()) {
                    return {};
                }
                SetDllDirectoryW(reinterpret_cast<LPCWSTR>(s_extractDir.utf16()));
                return s_extractDir;
            }
            // 有文件缺失/变化：继续执行提取（不清空目录，走增量提取分支）
        }
        markerFile.close();
    }

    // 创建目标目录；版本变化时先清空，确保不会残留旧版本的文件
    QDir extractDir(s_extractDir);
    if (!extractDir.exists()) {
        if (!QDir().mkpath(s_extractDir)) {
            return {};
        }
    } else if (versionChanged) {
        qDebug() << "ResourceExtractor: version changed, clearing" << s_extractDir;
        extractDir.removeRecursively();
        if (!QDir().mkpath(s_extractDir)) {
            return {};
        }
    }

    // 提取所有资源文件
    for (const auto& entry : kEntries) {
        QString destPath = s_extractDir + "/" + entry.diskName;

        // 增量提取：仅当"版本未变"且文件已存在时才跳过。
        // 版本变化时一律覆盖，避免旧 DLL 被留下来
        QFileInfo destInfo(destPath);
        if (!versionChanged && destInfo.exists()) {
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

    // 记录本次提取的清单，供下次启动判断内容是否变化
    writeManifest();

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
