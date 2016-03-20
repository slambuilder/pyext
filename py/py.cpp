#include "pch.h"
#include "PyDebugHelpers.h"

using namespace System;

void ManagedTest()
{

    System::Console::WriteLine("Module initialized.");
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

        ManagedTest();
    }
    catch (ExtException ex)
    {
        dprintf("Exception in extension 'py': hr=%#x, line=%d, function=%s\n", ex.hr, ex.line, ex.function);
    }

    return hr;
}

