#pragma once

#include "pch.h"

class ExtException
{
public:
    HRESULT hr;
    int line;
    const char *function;

    ExtException(HRESULT hr, int line, const char *function)
        : hr(hr)
        , line(line)
        , function(function)
    {
    }
};

#define HR_THROWIFFAIL(hr) if (FAILED(hr)) throw ExtException(hr, __LINE__, __FUNCTION__)

class PyDebugContext
{
private:
    IDebugClient5 *m_pClient;
    IDebugControl6 *m_pControl;
    IDebugSymbols5 *m_pSymbols;
public:
    PyDebugContext(IDebugClient5 *pClient)
        : m_pClient(nullptr)
        , m_pControl(nullptr)
        , m_pSymbols(nullptr)
    {
        HRESULT hr = S_OK;
        IDebugControl6 *pControl = nullptr;
        IDebugSymbols5 *pSymbols = nullptr;

        hr = pClient->QueryInterface(__uuidof(IDebugControl6), (void **)&pControl);
        if (SUCCEEDED(hr))
        {
            hr = pClient->QueryInterface(__uuidof(IDebugSymbols5), (void **)&pSymbols);
            if (SUCCEEDED(hr))
            {
                m_pClient = pClient;
                m_pControl = pControl; pControl = nullptr;
                m_pSymbols = pSymbols; pSymbols = nullptr;
            }
        }

        if (pControl) pControl->Release();
        if (pSymbols) pSymbols->Release();

        HR_THROWIFFAIL(hr);
    }

    ~PyDebugContext()
    {
        if (m_pControl) m_pControl->Release();
        if (m_pSymbols) m_pSymbols->Release();
    }

    IDebugClient5 *GetClient() const
    {
        return m_pClient;
    }

    IDebugControl6 *GetControl() const
    {
        return m_pControl;
    }

    IDebugSymbols5 *GetSymbols() const
    {
        return m_pSymbols;
    }
};



