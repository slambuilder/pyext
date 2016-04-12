#include "pch.h"
#include "PyDebugHelpers.h"
#include "WindowsUtils.h"

using namespace System;
using namespace IronPython;
using namespace IronPython::Hosting;

ref class AppDomainCustomResolve
{
private:
	static String^ s_ironPythonInstallPath;

public:
    static AppDomainCustomResolve()
    {
        auto currentDomain = System::AppDomain::CurrentDomain;

        currentDomain->AssemblyResolve += gcnew System::ResolveEventHandler(AppDomainCustomResolve::OnAssemblyResolve);

		std::wstring productCode;
		DWORD dwInstalledContext;
		std::wstring userSid;
		if (WindowsUtils::TryFindIronPythonProductCode(productCode, dwInstalledContext, userSid))
		{
			std::wstring installedLocation;
			if (WindowsUtils::TryFindInstalledLocation(productCode, dwInstalledContext, userSid, installedLocation))
			{
				s_ironPythonInstallPath = gcnew String(installedLocation.c_str());
			}
		}
	}

    static System::Reflection::Assembly^ OnAssemblyResolve(Object^ sender, ResolveEventArgs^ args)
    {
        if (args->Name->StartsWith("IronPython,") && s_ironPythonInstallPath != nullptr) {
            System::Reflection::Assembly^ pythonAssembly = 
				System::Reflection::Assembly::LoadFrom(s_ironPythonInstallPath + "\\IronPython.dll");
            return pythonAssembly;
        }
        else if (args->Name->StartsWith("Microsoft.Scripting,") && s_ironPythonInstallPath != nullptr) {
            System::Reflection::Assembly^ msScriptingAssembly = 
				System::Reflection::Assembly::LoadFrom(s_ironPythonInstallPath + "\\Microsoft.Scripting.dll");
            return msScriptingAssembly;
        }
		else if (args->Name->StartsWith("Microsoft.Dynamic,") && s_ironPythonInstallPath != nullptr) {
			System::Reflection::Assembly^ msDynamicAssembly =
				System::Reflection::Assembly::LoadFrom(s_ironPythonInstallPath + "\\Microsoft.Dynamic.dll");
			return msDynamicAssembly;
		}
		return nullptr;
    }
};

void InitializeAppDomainResolve()
{
	static bool s_initialized = false;

	if (!s_initialized)
	{
		AppDomainCustomResolve^ setup = gcnew AppDomainCustomResolve();
		s_initialized = true;
	}
}

void ManagedTest()
{
    Microsoft::Scripting::Hosting::ScriptEngine^ scriptEngine = nullptr;

    try
    {
        scriptEngine = IronPython::Hosting::Python::CreateEngine();
        System::Object^ obj = scriptEngine->Execute("2 + 2");

        System::String ^str = obj->ToString();
        System::Console::WriteLine(str);
    }
    finally
    {
        if (scriptEngine)
        {
            delete scriptEngine;
        }
    }
}

// A built-in help for the extension dll
HRESULT CALLBACK help(IDebugClient5 *pClient, PCSTR args)
{
    UNREFERENCED_PARAMETER(args);
    HRESULT hr = S_OK;

    try
    {
        PyDebugContext context(pClient);

        dprintf("Help for py.dll\n"
            "  py <command>         - Execute Python command\n"
            );
    }
    catch (ExtException ex)
    {
        dprintf("Exception in extension 'help': hr=%#x, line=%d, function=%s\n", ex.hr, ex.line, ex.function);
    }

    return hr;
}

HRESULT CALLBACK py(IDebugClient5 *pClient, PCSTR args)
{
    HRESULT hr = S_OK;

    try
    {
        PyDebugContext context(pClient);

        ULONG processorCount = 0;
        hr = context.GetControl()->GetNumberProcessors(&processorCount);
        HR_THROWIFFAIL(hr);

        ProcessorArchitecture actualProcessorType;
        hr = context.GetControl()->GetActualProcessorType(reinterpret_cast<ULONG*>(&actualProcessorType));
        HR_THROWIFFAIL(hr);

        ProcessorArchitecture effectiveProcessorType;
        hr = context.GetControl()->GetEffectiveProcessorType(reinterpret_cast<ULONG*>(&effectiveProcessorType));
        HR_THROWIFFAIL(hr);

        ProcessorArchitecture executingProcessorType;
        hr = context.GetControl()->GetExecutingProcessorType(reinterpret_cast<ULONG*>(&executingProcessorType));
        HR_THROWIFFAIL(hr);

        if (effectiveProcessorType == ProcessorArchitecture::X86)
        {
            __declspec(align(16)) PY_CONTEXT_X86 threadContext_x86;

            hr = context.GetAdvanced()->GetThreadContext(&threadContext_x86, sizeof(threadContext_x86));
            if (hr == E_INVALIDARG)
            {
                // limit context request to registers available in WOW64 only.
                threadContext_x86.ContextFlags = PY_WOW64_CONTEXT_CONTROL;
                hr = context.GetAdvanced()->GetThreadContext(&threadContext_x86, sizeof(threadContext_x86));
            }
            HR_THROWIFFAIL(hr);
        }
        else if (effectiveProcessorType == ProcessorArchitecture::X64)
        {
            __declspec(align(16)) PY_CONTEXT_X64 threadContext_x64;

            hr = context.GetAdvanced()->GetThreadContext(&threadContext_x64, sizeof(threadContext_x64));
            HR_THROWIFFAIL(hr);
        }

        InitializeAppDomainResolve();
        ManagedTest();

    }
    catch (ExtException ex)
    {
        dprintf("Exception in extension 'py': hr=%#x, line=%d, function=%s\n", ex.hr, ex.line, ex.function);
    }

    return hr;
}

