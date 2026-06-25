#include "NativeLibraryLoader.h"

bool NativeLibraryLoader::load(const QString& dllPath,
                               const std::vector<std::pair<const char*, void**>>& functions)
{
	if (m_state == State::Loaded) return true;

	m_library.setFileName(dllPath);
	if (!m_library.load())
	{
		m_state = State::LoadFailed;
		m_errorString = m_library.errorString();
		return false;
	}

	for (auto& [name, target] : functions)
	{
		QFunctionPointer fn = m_library.resolve(name);
		if (!fn)
		{
			m_state = State::ResolveFailed;
			m_errorString = QString("Failed to resolve: %1").arg(name);
			return false;
		}
		*target = reinterpret_cast<void*>(fn);
	}

	m_state = State::Loaded;
	return true;
}

void NativeLibraryLoader::unload()
{
	if (m_library.isLoaded())
		m_library.unload();
	m_state = State::NotLoaded;
}
