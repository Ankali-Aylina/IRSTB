#pragma once

#include <QString>

/// <summary>
/// 从 Qt 资源系统（QRC）中提取运行时所需的文件到临时目录。
/// DLL 文件通过 NativeLibraryLoader 从文件系统加载，必须提取到磁盘。
/// </summary>
class ResourceExtractor {
public:
    /// <summary>提取所有运行时资源到临时目录，并设置 DLL 搜索路径</summary>
    /// <returns>临时目录路径，失败返回空字符串</returns>
    static QString extract();

    /// <summary>获取已提取文件的完整路径</summary>
    /// <remarks>必须在 extract() 成功后调用</remarks>
    static QString filePath(const QString& relativePath);
};
