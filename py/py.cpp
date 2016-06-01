#include "pch.h"
#include "PyDebugHelpers.h"
#include "WindowsUtils.h"

using namespace System;
using namespace System::Dynamic;
using namespace System::IO;
using namespace System::Reflection;
using namespace IronPython;
using namespace IronPython::Hosting;

using namespace System::Runtime::InteropServices;

private ref class PyExtensionGlobals : public IDynamicMetaObjectProvider
{
public:
    virtual DynamicMetaObject^ GetMetaObject(System::Linq::Expressions::Expression^ parameter)
    {
        return nullptr;
    }
};

ref class AppDomainCustomResolve
{
private:
	static String^ s_ironPythonInstallPath;

public:
    static AppDomainCustomResolve()
    {
        auto currentDomain = AppDomain::CurrentDomain;

        currentDomain->AssemblyResolve += gcnew ResolveEventHandler(AppDomainCustomResolve::OnAssemblyResolve);

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

        if (!s_ironPythonInstallPath)
        {
            // use current assembly location to load local IronPython modules
            auto codeBase = Assembly::GetExecutingAssembly()->CodeBase;
            auto uri = gcnew UriBuilder(codeBase);
            auto path = Uri::UnescapeDataString(uri->Path);
            s_ironPythonInstallPath = Path::GetDirectoryName(path);
        }
	}

    static System::Reflection::Assembly^ OnAssemblyResolve(Object^ sender, ResolveEventArgs^ args)
    {
        if (args->Name->StartsWith("IronPython,") && s_ironPythonInstallPath != nullptr) {
            Assembly^ pythonAssembly = 
				Assembly::LoadFrom(s_ironPythonInstallPath + "\\IronPython.dll");
            return pythonAssembly;
        }
        else if (args->Name->StartsWith("Microsoft.Scripting,") && s_ironPythonInstallPath != nullptr) {
            Assembly^ msScriptingAssembly = 
				Assembly::LoadFrom(s_ironPythonInstallPath + "\\Microsoft.Scripting.dll");
            return msScriptingAssembly;
        }
		else if (args->Name->StartsWith("Microsoft.Dynamic,") && s_ironPythonInstallPath != nullptr) {
			Assembly^ msDynamicAssembly =
				Assembly::LoadFrom(s_ironPythonInstallPath + "\\Microsoft.Dynamic.dll");
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

ref class PyGlobals
{
public:
    static Microsoft::Scripting::Hosting::ScriptEngine^ s_scriptEngine = nullptr;
    static Microsoft::Scripting::Hosting::ScriptScope^ s_scriptScope = nullptr;

    static void InitializeGlobals()
    {
        if (!s_scriptEngine) 
        {
            // TODO: create appdomain
            s_scriptEngine = IronPython::Hosting::Python::CreateEngine();
            s_scriptScope = s_scriptEngine->CreateScope();
        }
    }
};

void ExecutePythonCommand(PyDebugContext &context, PCSTR args)
{
    try
    {
        PyGlobals::InitializeGlobals();

        auto obj2 = PyGlobals::s_scriptEngine->Execute(gcnew System::String(args), PyGlobals::s_scriptScope);
        if (obj2)
        {
            System::Console::WriteLine(obj2->ToString());
        }
    }
    catch (Exception^ ex)
    {
        IntPtr ptrExeptionMessage = Marshal::StringToHGlobalAnsi(ex->ToString());
        dprintf("Exception: %s\n", static_cast<char*>(ptrExeptionMessage.ToPointer()));
        Marshal::FreeHGlobal(ptrExeptionMessage);
    }
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

        ::ProcessorArchitecture actualProcessorType;
        hr = context.GetControl()->GetActualProcessorType(reinterpret_cast<ULONG*>(&actualProcessorType));
        HR_THROWIFFAIL(hr);

        ::ProcessorArchitecture effectiveProcessorType;
        hr = context.GetControl()->GetEffectiveProcessorType(reinterpret_cast<ULONG*>(&effectiveProcessorType));
        HR_THROWIFFAIL(hr);

        ::ProcessorArchitecture executingProcessorType;
        hr = context.GetControl()->GetExecutingProcessorType(reinterpret_cast<ULONG*>(&executingProcessorType));
        HR_THROWIFFAIL(hr);

        if (effectiveProcessorType == ::ProcessorArchitecture::X86)
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
        else if (effectiveProcessorType == ::ProcessorArchitecture::X64)
        {
            __declspec(align(16)) PY_CONTEXT_X64 threadContext_x64;

            hr = context.GetAdvanced()->GetThreadContext(&threadContext_x64, sizeof(threadContext_x64));
            HR_THROWIFFAIL(hr);
        }

        InitializeAppDomainResolve();
        ExecutePythonCommand(context, args);
    }
    catch (ExtException ex)
    {
        dprintf("Exception in extension 'py': hr=%#x, line=%d, function=%s\n", ex.hr, ex.line, ex.function);
    }

    return hr;
}
