#include "pch.h"
#include "WindowsUtils.h"
#include <Msi.h>

bool WindowsUtils::TryFindIronPythonProductCode(
	std::wstring & productCode, 
	DWORD & dwInstalledContext, 
	std::wstring & sid)
{
	DWORD dwIndex = 0;
	bool found = false;

	while (true)
	{
		MSIINSTALLCONTEXT context = MSIINSTALLCONTEXT_NONE;

		wchar_t productCodeBuffer[40];
		wchar_t userSidBuffer[184];
		DWORD cchUserSidBuffer = _countof(userSidBuffer);

		UINT result = MsiEnumProductsExW(
			NULL,
			NULL,
			MSIINSTALLCONTEXT_ALL,
			dwIndex,
			productCodeBuffer,
			&context,
			userSidBuffer,
			&cchUserSidBuffer);

		dwIndex++;

		if (result == ERROR_MORE_DATA || 
			result == ERROR_BAD_CONFIGURATION || 
			result == ERROR_ACCESS_DENIED ||
			result == ERROR_UNKNOWN_PRODUCT)
		{
			// current product has issues, continue enumerating
			continue;
		}

		if (result != ERROR_SUCCESS)
		{
			break;
		}

		wchar_t productNameBuffer[256];
		DWORD cchProductName = _countof(productNameBuffer);
		result = MsiGetProductInfoExW(productCodeBuffer,
			userSidBuffer[0] == 0 ? NULL : userSidBuffer,
			context,
			INSTALLPROPERTY_INSTALLEDPRODUCTNAME,
			productNameBuffer,
			&cchProductName);

		if (result != ERROR_SUCCESS)
		{
			continue;
		}

		if (_wcsnicmp(productNameBuffer, L"IronPython", _countof("IronPython")-1) == 0)
		{
			std::wstring userSid;

			// Found IronPython

			dwInstalledContext = context;
			productCode = productCodeBuffer;
			userSid = userSidBuffer;

			found = true;
			break;
		}
	}
	return found;
}

bool WindowsUtils::TryFindInstalledLocation(
	const std::wstring & productCode, 
	DWORD dwInstalledContext, 
	const std::wstring & sid, 
	std::wstring &installedLocation)
{
	wchar_t installedLocationBuffer[1024];
	DWORD cchProductName = _countof(installedLocationBuffer);
	UINT result = MsiGetProductInfoExW(productCode.c_str(),
		sid.length() == 0 ? NULL : sid.c_str(),
		static_cast<MSIINSTALLCONTEXT>(dwInstalledContext),
		INSTALLPROPERTY_INSTALLLOCATION,
		installedLocationBuffer,
		&cchProductName);

	if (result == ERROR_SUCCESS)
	{
		installedLocation = installedLocationBuffer;
		return true;
	}

	return false;
}
