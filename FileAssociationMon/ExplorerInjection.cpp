#include "ExplorerInjection.h"
#include <Shlwapi.h>
#include "../FileAssociationHelperUtils/APIHook/Hook.h"
#include "InjectionFactory.h"
#include "../FileAssociationService/FileAssociationService_h.h"
#include "CommandExecute.h"

#pragma comment(lib, "Shlwapi.lib")

// system Open Unknown file Cmdline Flag
#define SYSCMDFLAG L"shell32.dll,OpenAs_RunDLL"
// Open with me Open Unknown file Cmdline Flag
#define OWMCMDFLAG L"OpenWith.exe\""
// Advanced System Protector Open Unknown Cmdline Flag
#define ASPCMDFLAG L"filetypehelper.exe\" -scanunknown"

ExplorerInjection::CREATEPROCESSWDEF ExplorerInjection::realcreateprocessw_ = nullptr;
ExplorerInjection::COCREATEINSTANCEDEF ExplorerInjection::realcocreateinstance_ = nullptr;
const wchar_t *ExplorerInjection::processname_ = L"explorer.exe";
volatile long ExplorerInjection::ref_ = 0;

bool ExplorerInjection::InjectProcess(DWORD processid)
{
    if (processid)
        return InjectionFactory::InjectProcess(processid, ExplorerInjection::BeforeMonitor, ExplorerInjection::AfterMonitor, ExplorerInjection::UninjecTest);
    else 
        return InjectionFactory::InjectProcess(ExplorerInjection::processname_, ExplorerInjection::BeforeMonitor, ExplorerInjection::AfterMonitor, ExplorerInjection::UninjecTest);
}

bool ExplorerInjection::UninjectProcess(DWORD processid)
{
    if (processid)
        return InjectionFactory::UninjectProcess(processid);
    else
        return InjectionFactory::UninjectProcess(ExplorerInjection::processname_);
}

bool WINAPI ExplorerInjection::BeforeMonitor()
{
    return HookCreateProcessW() && HookCoCreateInstance();
}

bool WINAPI ExplorerInjection::AfterMonitor()
{
    if (realcreateprocessw_)
        UnhookCreateProcessW();
    if (realcocreateinstance_)
        UnHookCoCreateInstance();
    return true;
}

bool WINAPI ExplorerInjection::UninjecTest()
{
    return ref_ == 0;
}

BOOL WINAPI ExplorerInjection::CreateProcessW(
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
    )
{
    ::InterlockedIncrement(&ExplorerInjection::ref_);
    BOOL bRet = FALSE;
    LPWSTR filepath = nullptr;
    if(ContainsInterceptFlag(lpApplicationName, lpCommandLine, &filepath))
    {
        ::CoInitialize(0);
        {
            IFileAssociationHelper *helper;
            HRESULT hr = ::CoCreateInstance(__uuidof(FileAssociationHelper), nullptr, CLSCTX_LOCAL_SERVER, __uuidof(IFileAssociationHelper), (LPVOID *)&helper);
            if (FAILED(hr))
            {
                bRet = ExplorerInjection::realcreateprocessw_(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
            }
            else
            {
                BSTR bstrfilepath = ::SysAllocString(filepath);
                HRESULT hr = helper->OpenHelperDialog(bstrfilepath);
                bRet = hr == S_OK;
                ::SysFreeString(bstrfilepath);
                helper->Release();
            }
        }
        ::CoUninitialize();
    }
    else
    {
        bRet = ExplorerInjection::realcreateprocessw_(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation) ;
    }
    ::InterlockedDecrement(&ExplorerInjection::ref_);
    return bRet;
}

bool ExplorerInjection::HookCreateProcessW()
{
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    CREATEPROCESSWDEF target = (CREATEPROCESSWDEF)GetProcAddress(kernel32, "CreateProcessW");

    realcreateprocessw_ = apihook::Hook(target, ExplorerInjection::CreateProcessW);
    return realcreateprocessw_ != nullptr;
}

bool ExplorerInjection::UnhookCreateProcessW()
{
    return apihook::Unhook(realcreateprocessw_, ExplorerInjection::CreateProcessW);
}

HRESULT WINAPI ExplorerInjection::CoCreateInstance(
    _In_   REFCLSID rclsid,
    _In_   LPUNKNOWN pUnkOuter,
    _In_   DWORD dwClsContext,
    _In_   REFIID riid,
    _Out_  LPVOID *ppv
    )
{
    ::InterlockedIncrement(&ExplorerInjection::ref_);
    HRESULT hr = S_FALSE;
#if defined(_WIN32_WINNT_WIN8)
    if (rclsid == CLSID_ExecuteUnknown)
    {
        hr = ExplorerInjection::realcocreateinstance_(rclsid, pUnkOuter, dwClsContext, riid, ppv);
        if (hr == S_OK)
        {
            hr = CommandExecuteImpl::CreateInstance((IExecuteCommand **)ppv) ? S_OK: S_FALSE;
        }
    }
    else
#endif
        hr = ExplorerInjection::realcocreateinstance_(rclsid, pUnkOuter, dwClsContext, riid, ppv);
    ::InterlockedDecrement(&ExplorerInjection::ref_);
    return hr;
}

bool ExplorerInjection::HookCoCreateInstance()
{
    HMODULE kernel32 = GetModuleHandleW(L"Ole32.dll");
    COCREATEINSTANCEDEF target = (COCREATEINSTANCEDEF) GetProcAddress(kernel32, "CoCreateInstance");

    realcocreateinstance_ = apihook::Hook(target, ExplorerInjection::CoCreateInstance);
    return realcocreateinstance_ != nullptr;
}

bool ExplorerInjection::UnHookCoCreateInstance()
{
    return apihook::Unhook(realcocreateinstance_, ExplorerInjection::CoCreateInstance);
}

bool ExplorerInjection::ContainsInterceptFlag(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPWSTR *fileName)
{
    LPCWSTR cmdFlags[] = {SYSCMDFLAG, OWMCMDFLAG, ASPCMDFLAG, nullptr};
    LPCWSTR *cmdFlag = nullptr;
    WCHAR *pos = nullptr;
    for (cmdFlag = cmdFlags; cmdFlag && *cmdFlag; ++cmdFlag)
    {
        if ((pos = StrStrI(lpCommandLine, *cmdFlag)) != nullptr)
            break;
    }
    if(pos != nullptr){
        *fileName = (LPWSTR)(pos + wcslen(*cmdFlag) + 1); //skip space
        return true;
    }

    return false;
}