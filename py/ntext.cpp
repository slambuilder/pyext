#include "pch.h"
#include "PyDebugHelpers.h"

// Global variables
bool g_connected = false;
ULONG g_ulTargetMachine = 0;

WINDBG_EXTENSION_APIS ExtensionApis;

// Export function
// The DebugExtensionInitialize callback function is called by the engine after loading a DbgEng extension DLL.
extern "C"
HRESULT CALLBACK DebugExtensionInitialize(_Out_ PULONG pVersion, _Out_ PULONG pFlags)
{
    IDebugClient5 *pDebugClient = nullptr;
    IDebugControl6 *pDebugControl = nullptr;
    HRESULT hr = S_OK;

    *pVersion = DEBUG_EXTENSION_VERSION(1, 0);
    *pFlags = 0;

    memset(&ExtensionApis, 0, sizeof(ExtensionApis));

    try
    {
        hr = DebugCreate(__uuidof(IDebugClient5), (void **)&pDebugClient);
        HR_THROWIFFAIL(hr);

        hr = pDebugClient->QueryInterface(__uuidof(IDebugControl6), (void **)&pDebugControl);
        HR_THROWIFFAIL(hr);

        //
        // Get the windbg-style extension APIS
        //
        ExtensionApis.nSize = sizeof(ExtensionApis);
        hr = pDebugControl->GetWindbgExtensionApis64(&ExtensionApis);
        HR_THROWIFFAIL(hr);
    }
    catch (ExtException ex)
    {
        // we cannot print anything here.
        hr = ex.hr;
    }

    if (pDebugClient) pDebugClient->Release();
    if (pDebugControl) pDebugControl->Release();

    return hr;
}

// Export function
// The engine calls the DebugExtensionNotify callback function to inform the extension DLL when a session changes its active or accessible status.
extern "C"
void CALLBACK DebugExtensionNotify(ULONG Notify, ULONG64 Argument)
{
    UNREFERENCED_PARAMETER(Argument);

    //
    // The first time we actually connect to a target
    //

    if ((Notify == DEBUG_NOTIFY_SESSION_ACCESSIBLE) && (!g_connected))
    {
        IDebugClient5 *pDebugClient = nullptr;
        IDebugControl6 *pDebugControl = nullptr;
        HRESULT hr = S_OK;
        try
        {
            hr = DebugCreate(__uuidof(IDebugClient5), (void **)&pDebugClient);
            HR_THROWIFFAIL(hr);

            // Get the architecture type.
            hr = pDebugClient->QueryInterface(__uuidof(IDebugControl6), (void **)&pDebugControl);
            HR_THROWIFFAIL(hr);

            hr = pDebugControl->GetActualProcessorType(&g_ulTargetMachine);
            HR_THROWIFFAIL(hr);

            g_connected = true;

            // NotifyOnTargetAccessible(pDebugControl);

        }
        catch (ExtException ex)
        {
            hr = ex.hr;
        }

        if (pDebugControl) pDebugControl->Release();
        if (pDebugClient) pDebugClient->Release();
    }

    if (Notify == DEBUG_NOTIFY_SESSION_INACTIVE)
    {
        g_connected = false;
        g_ulTargetMachine = 0;
    }

    return;
}

// export function
// The DebugExtensionUninitialize callback function is called by the engine to uninitialize the DbgEng extension DLL before it is unloaded.
extern "C"
void CALLBACK DebugExtensionUninitialize(void)
{
    return;
}

// This gets called (by DebugExtensionNotify whentarget is halted and is accessible
HRESULT NotifyOnTargetAccessible(IDebugControl6 *pControl)
{
    dprintf("Extension dll detected a break");
    if (g_connected) {
        dprintf(" connected to ");
        switch (g_ulTargetMachine) {
        case IMAGE_FILE_MACHINE_I386:
            dprintf("X86");
            break;
        case IMAGE_FILE_MACHINE_IA64:
            dprintf("IA64");
            break;
        default:
            dprintf("Other");
            break;
        }
    }
    dprintf("\n");

    //
    // show the top frame and execute dv to dump the locals here and return
    //
    pControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS |
        DEBUG_OUTCTL_OVERRIDE_MASK |
        DEBUG_OUTCTL_NOT_LOGGED,
        ".frame", // Command to be executed
        DEBUG_EXECUTE_DEFAULT);
    pControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS |
        DEBUG_OUTCTL_OVERRIDE_MASK |
        DEBUG_OUTCTL_NOT_LOGGED,
        "dv", // Command to be executed
        DEBUG_EXECUTE_DEFAULT);
    return S_OK;
}

