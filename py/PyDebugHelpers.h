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
    IDebugAdvanced3 *m_pAdvanced;
    IDebugRegisters2 *m_pRegisters;
    IDebugSystemObjects4 *m_pSystemObjects;

public:
    PyDebugContext(IDebugClient5 *pClient)
        : m_pClient(nullptr)
        , m_pControl(nullptr)
        , m_pSymbols(nullptr)
        , m_pAdvanced(nullptr)
        , m_pRegisters(nullptr)
        , m_pSystemObjects(nullptr)
    {
        HRESULT hr = S_OK;
        IDebugControl6 *pControl = nullptr;
        IDebugSymbols5 *pSymbols = nullptr;
        IDebugAdvanced3 *pAdvanced = nullptr;
        IDebugRegisters2 *pRegisters = nullptr;
        IDebugSystemObjects4 *pSystemObjects = nullptr;

        hr = pClient->QueryInterface(__uuidof(IDebugControl6), (void **)&pControl);
        if (SUCCEEDED(hr))
        {
            hr = pClient->QueryInterface(__uuidof(IDebugSymbols5), (void **)&pSymbols);
            if (SUCCEEDED(hr))
            {
                hr = pClient->QueryInterface(__uuidof(IDebugAdvanced3), (void **)&pAdvanced);
                if (SUCCEEDED(hr))
                {
                    hr = pClient->QueryInterface(__uuidof(IDebugRegisters2), (void **)&pRegisters);
                    if (SUCCEEDED(hr))
                    {
                        hr = pClient->QueryInterface(__uuidof(IDebugSystemObjects4), (void **)&pSystemObjects);
                        if (SUCCEEDED(hr))
                        {
                            m_pClient = pClient;
                            m_pControl = pControl; pControl = nullptr;
                            m_pSymbols = pSymbols; pSymbols = nullptr;
                            m_pAdvanced = pAdvanced; pAdvanced = nullptr;
                            m_pRegisters = pRegisters; pRegisters = nullptr;
                            m_pSystemObjects = pSystemObjects; pSystemObjects = nullptr;
                        }
                    }
                }
            }
        }

        if (pControl) pControl->Release();
        if (pSymbols) pSymbols->Release();
        if (pAdvanced) pAdvanced->Release();
        if (pRegisters) pRegisters->Release();
        if (pSystemObjects) pSystemObjects->Release();

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

    IDebugAdvanced3 *GetAdvanced() const
    {
        return m_pAdvanced;
    }

    IDebugRegisters2 *GetRegisters() const
    {
        return m_pRegisters;
    }

    IDebugSystemObjects4 *GetSystemObjects() const
    {
        return m_pSystemObjects;
    }
};

enum class ProcessorArchitecture : ULONG
{
    X86 = IMAGE_FILE_MACHINE_I386,
    ARM = IMAGE_FILE_MACHINE_ARM,
    IA64 = IMAGE_FILE_MACHINE_IA64,
    X64 = IMAGE_FILE_MACHINE_AMD64,
    EfiByteCode = IMAGE_FILE_MACHINE_EBC,
};

// ---------------------------------------------------------------------
// Copy of definition of 32-bit and 64-bit CONTEXT records from winnt.h
// We need to have a duplicates, so we can operate on both 
// 32 and 64 bits from any process architecture
// ---------------------------------------------------------------------

#pragma pack(push,4)

#define PY_MAXIMUM_SUPPORTED_EXTENSION_X86     512

//
//  Define the size of the 80387 save area, which is in the context frame.
//

#define PY_SIZE_OF_80387_REGISTERS_X86      80

struct PY_FLOATING_SAVE_AREA_X86 {
    DWORD   ControlWord;
    DWORD   StatusWord;
    DWORD   TagWord;
    DWORD   ErrorOffset;
    DWORD   ErrorSelector;
    DWORD   DataOffset;
    DWORD   DataSelector;
    BYTE    RegisterArea[PY_SIZE_OF_80387_REGISTERS_X86];
    DWORD   Spare0;
};

#define PY_CONTEXT_i386 0x00010000L
#define PY_WOW64_CONTEXT_CONTROL (PY_CONTEXT_i386 | 0x00000001L) // SS:SP, CS:IP, FLAGS, BP

//
// Context Frame for x86
//
struct PY_CONTEXT_X86 {

    //
    // The flags values within this flag control the contents of
    // a CONTEXT record.
    //
    // If the context record is used as an input parameter, then
    // for each portion of the context record controlled by a flag
    // whose value is set, it is assumed that that portion of the
    // context record contains valid context. If the context record
    // is being used to modify a threads context, then only that
    // portion of the threads context will be modified.
    //
    // If the context record is used as an IN OUT parameter to capture
    // the context of a thread, then only those portions of the thread's
    // context corresponding to set flags will be returned.
    //
    // The context record is never used as an OUT only parameter.
    //

    DWORD ContextFlags;

    //
    // This section is specified/returned if CONTEXT_DEBUG_REGISTERS is
    // set in ContextFlags.  Note that CONTEXT_DEBUG_REGISTERS is NOT
    // included in CONTEXT_FULL.
    //

    DWORD   Dr0;
    DWORD   Dr1;
    DWORD   Dr2;
    DWORD   Dr3;
    DWORD   Dr6;
    DWORD   Dr7;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_FLOATING_POINT.
    //

    PY_FLOATING_SAVE_AREA_X86 FloatSave;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_SEGMENTS.
    //

    DWORD   SegGs;
    DWORD   SegFs;
    DWORD   SegEs;
    DWORD   SegDs;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_INTEGER.
    //

    DWORD   Edi;
    DWORD   Esi;
    DWORD   Ebx;
    DWORD   Edx;
    DWORD   Ecx;
    DWORD   Eax;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_CONTROL.
    //

    DWORD   Ebp;
    DWORD   Eip;
    DWORD   SegCs;              // MUST BE SANITIZED
    DWORD   EFlags;             // MUST BE SANITIZED
    DWORD   Esp;
    DWORD   SegSs;

    //
    // This section is specified/returned if the ContextFlags word
    // contains the flag CONTEXT_EXTENDED_REGISTERS.
    // The format and contexts are processor specific
    //

    BYTE    ExtendedRegisters[PY_MAXIMUM_SUPPORTED_EXTENSION_X86];

};

#pragma pack(pop)

//
// Format of data for (F)XSAVE/(F)XRSTOR instruction
//

struct DECLSPEC_ALIGN(16) PY_XSAVE_FORMAT_X86 {
    WORD   ControlWord;
    WORD   StatusWord;
    BYTE  TagWord;
    BYTE  Reserved1;
    WORD   ErrorOpcode;
    DWORD ErrorOffset;
    WORD   ErrorSelector;
    WORD   Reserved2;
    DWORD DataOffset;
    WORD   DataSelector;
    WORD   Reserved3;
    DWORD MxCsr;
    DWORD MxCsr_Mask;
    M128A FloatRegisters[8];

    M128A XmmRegisters[8];
    BYTE  Reserved4[224];
};

struct DECLSPEC_ALIGN(16) PY_XSAVE_FORMAT_X64 {
    WORD   ControlWord;
    WORD   StatusWord;
    BYTE  TagWord;
    BYTE  Reserved1;
    WORD   ErrorOpcode;
    DWORD ErrorOffset;
    WORD   ErrorSelector;
    WORD   Reserved2;
    DWORD DataOffset;
    WORD   DataSelector;
    WORD   Reserved3;
    DWORD MxCsr;
    DWORD MxCsr_Mask;
    M128A FloatRegisters[8];

    M128A XmmRegisters[16];
    BYTE  Reserved4[96];
};

// end_wdm end_ntosp
// begin_ntddk

//
// Context Frame
//
//  This frame has a several purposes: 1) it is used as an argument to
//  NtContinue, 2) it is used to constuct a call frame for APC delivery,
//  and 3) it is used in the user level thread creation routines.
//
//
// The flags field within this record controls the contents of a CONTEXT
// record.
//
// If the context record is used as an input parameter, then for each
// portion of the context record controlled by a flag whose value is
// set, it is assumed that that portion of the context record contains
// valid context. If the context record is being used to modify a threads
// context, then only that portion of the threads context is modified.
//
// If the context record is used as an output parameter to capture the
// context of a thread, then only those portions of the thread's context
// corresponding to set flags will be returned.
//
// CONTEXT_CONTROL specifies SegSs, Rsp, SegCs, Rip, and EFlags.
//
// CONTEXT_INTEGER specifies Rax, Rcx, Rdx, Rbx, Rbp, Rsi, Rdi, and R8-R15.
//
// CONTEXT_SEGMENTS specifies SegDs, SegEs, SegFs, and SegGs.
//
// CONTEXT_FLOATING_POINT specifies Xmm0-Xmm15.
//
// CONTEXT_DEBUG_REGISTERS specifies Dr0-Dr3 and Dr6-Dr7.
//

struct __declspec(align(16)) PY_CONTEXT_X64 {

    //
    // Register parameter home addresses.
    //
    // N.B. These fields are for convience - they could be used to extend the
    //      context record in the future.
    //

    DWORD64 P1Home;
    DWORD64 P2Home;
    DWORD64 P3Home;
    DWORD64 P4Home;
    DWORD64 P5Home;
    DWORD64 P6Home;

    //
    // Control flags.
    //

    DWORD ContextFlags;
    DWORD MxCsr;

    //
    // Segment Registers and processor flags.
    //

    WORD   SegCs;
    WORD   SegDs;
    WORD   SegEs;
    WORD   SegFs;
    WORD   SegGs;
    WORD   SegSs;
    DWORD EFlags;

    //
    // Debug registers
    //

    DWORD64 Dr0;
    DWORD64 Dr1;
    DWORD64 Dr2;
    DWORD64 Dr3;
    DWORD64 Dr6;
    DWORD64 Dr7;

    //
    // Integer registers.
    //

    DWORD64 Rax;
    DWORD64 Rcx;
    DWORD64 Rdx;
    DWORD64 Rbx;
    DWORD64 Rsp;
    DWORD64 Rbp;
    DWORD64 Rsi;
    DWORD64 Rdi;
    DWORD64 R8;
    DWORD64 R9;
    DWORD64 R10;
    DWORD64 R11;
    DWORD64 R12;
    DWORD64 R13;
    DWORD64 R14;
    DWORD64 R15;

    //
    // Program counter.
    //

    DWORD64 Rip;

    //
    // Floating point state.
    //

    union {
        PY_XSAVE_FORMAT_X64 FltSave;
        struct {
            M128A Header[2];
            M128A Legacy[8];
            M128A Xmm0;
            M128A Xmm1;
            M128A Xmm2;
            M128A Xmm3;
            M128A Xmm4;
            M128A Xmm5;
            M128A Xmm6;
            M128A Xmm7;
            M128A Xmm8;
            M128A Xmm9;
            M128A Xmm10;
            M128A Xmm11;
            M128A Xmm12;
            M128A Xmm13;
            M128A Xmm14;
            M128A Xmm15;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;

    //
    // Vector registers.
    //

    M128A VectorRegister[26];
    DWORD64 VectorControl;

    //
    // Special debug control registers.
    //

    DWORD64 DebugControl;
    DWORD64 LastBranchToRip;
    DWORD64 LastBranchFromRip;
    DWORD64 LastExceptionToRip;
    DWORD64 LastExceptionFromRip;
};



