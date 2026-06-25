#include "AppModuleManager.h"
#include "TCCore.h"
#include "BLEThread.h"

AppModuleManager::AppModuleManager(QObject* parent)
    : QObject(parent)
{
}

AppModuleManager::~AppModuleManager()
{
    stopAll();
}

bool AppModuleManager::initializeAll()
{
    for (auto& m : m_modules)
    {
        if (!m->initialize())
            return false;
    }
    return true;
}

void AppModuleManager::startAll()
{
    for (auto& m : m_modules)
    {
        m->start();
    }
}

void AppModuleManager::stopAll(int timeoutMs)
{
    // 逆序停止，确保依赖先停
    for (auto it = m_modules.rbegin(); it != m_modules.rend(); ++it)
    {
        (*it)->stop(timeoutMs);
    }
}
