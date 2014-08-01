#ifndef EXPLORERINJECT_H_
#define EXPLORERINJECT_H_

#include <Windows.h>
#include "FileAssociationMon.h"

class FILEASSOCIATIONMON_API ExplorerInjection
{
private:
    typedef BOOL (WINAPI *CREATEPROCESSWDEF)(
        LPCWSTR lpApplicationName,
        LPWSTR lpCommandLine,
        LPSECURITY_ATTRIBUTES lpProcessAttributes,
        LPSECURITY_ATTRIBUTES lpThreadAttributes,
        BOOL bInheritHandles,
        DWORD dwCreationFlags,
        LPVOID lpEnvironment,
        LPCWSTR lpCurrentDirectory,
        LPSTARTUPINFOW lpStartupInfo,
        LPPROCESS_INFORMATION lpProcessInformation
        );
    static CREATEPROCESSWDEF realcreateprocessw_;

    typedef HRESULT (WINAPI *COCREATEINSTANCEDEF)(
        REFCLSID rclsid,
        LPUNKNOWN pUnkOuter,
        DWORD dwClsContext,
        REFIID riid,
        LPVOID *ppv
        );
    static COCREATEINSTANCEDEF realcocreateinstance_;

public:
    static bool InjectProcess(DWORD processid = 0);
    static bool UninjectProcess(DWORD processid = 0);
    static const wchar_t *processname_;
    static volatile long ref_;

private:
    static bool WINAPI BeforeMonitor();
    static bool WINAPI AfterMonitor();
    static bool WINAPI UninjecTest();

    static BOOL WINAPI CreateProcessW(
        LPCWSTR lpApplicationName,
        LPWSTR lpCommandLine,
        LPSECURITY_ATTRIBUTES lpProcessAttributes,
        LPSECURITY_ATTRIBUTES lpThreadAttributes,
        BOOL bInheritHandles,
        DWORD dwCreationFlags,
        LPVOID lpEnvironment,
        LPCWSTR lpCurrentDirectory,
        LPSTARTUPINFOW lpStartupInfo,
        LPPROCESS_INFORMATION lpProcessInformation
        );
    static bool HookCreateProcessW(void);
    static bool UnhookCreateProcessW(void);

    static HRESULT WINAPI CoCreateInstance(
        _In_   REFCLSID rclsid,
        _In_   LPUNKNOWN pUnkOuter,
        _In_   DWORD dwClsContext,
        _In_   REFIID riid,
        _Out_  LPVOID *ppv
        );
    static bool HookCoCreateInstance();
    static bool UnHookCoCreateInstance();

    static bool ContainsInterceptFlag(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPWSTR *fileName);
};

#endif