#include "pch.h"
#include "PyDebugHelpers.h"

/*
A built-in help for the extension dll
*/
HRESULT CALLBACK help(IDebugClient5 *pClient, PCSTR args)
{
    UNREFERENCED_PARAMETER(args);
    HRESULT hr = S_OK;

    try
    {
        PyDebugContext context(pClient);

        dprintf("Help for py.dll\n"
            "  cmdsample           - This does stacktrace and lists\n"
            "  help                = Shows this help\n"
            "  structsample <addr> - This dumps a struct at given address\n"
            );
    }
    catch (ExtException ex)
    {
        dprintf("Exception in extension 'help': hr=%#x, line=%d, function=%s", ex.hr, ex.line, ex.function);
    }

    return S_OK;
}
