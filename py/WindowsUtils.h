#pragma once

#include <string>

// collection utility functions specific to Windows environment
class WindowsUtils
{
public:
	static bool TryFindIronPythonProductCode(
		std::wstring &productCode,
		DWORD &dwInstalledContext,
		std::wstring &sid);

	static bool TryFindInstalledLocation(
		const std::wstring &productCode,
		DWORD dwInstalledContext,
		const std::wstring &sid,
		std::wstring &installedLocation);
};

