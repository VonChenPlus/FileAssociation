#include "ProcessInjection.h"
#include <TlHelp32.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;

BOOL ProcessInjection::InjectProcess(DWORD processid, const RemoteAction &action)
{
    BOOL result = FALSE;
    HANDLE process = nullptr;

    do
    {
        process = ::OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION 
            |PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, processid);
        if (process == nullptr) break;

        HMODULE module = RemoteLoadLibrary(process);
        if (module == nullptr) break;

        result = RemoteExecute(process, module, action);

        RemoteFreeLibrary(process, module);
    } while(false);

    if (process)
        ::CloseHandle(process);

    return result;
}

HMODULE ProcessInjection::RemoteLoadLibrary(HANDLE process)
{
    if (process == nullptr) return nullptr;

    HMODULE injectmodule = nullptr;
    void *virmemptr = nullptr;
    HANDLE remotethread = nullptr;

    do
    {
        wchar_t modulepath[MAX_PATH];
        ::GetModuleFileNameW((HMODULE)&__ImageBase, modulepath, MAX_PATH);
        if (WriteProcessMemory(process, modulepath, MAX_PATH * sizeof(wchar_t), &virmemptr) == FALSE) break;

        void *loadlibaryptr = GetSysLoadLibaryFuncPtr();
        if (loadlibaryptr == nullptr) break;
        remotethread = ::CreateRemoteThread(process, nullptr, 0, (LPTHREAD_START_ROUTINE)loadlibaryptr, virmemptr, 0, nullptr);
        if (remotethread == nullptr) break;

        ::WaitForSingleObject(remotethread, INFINITE);
        injectmodule = GetProcessModuleHandle(remotethread, modulepath);
    } while(false);

    if (virmemptr)
        ::VirtualFreeEx(process, virmemptr, 0, MEM_RELEASE);
    if (remotethread)
        ::CloseHandle(remotethread);

    return injectmodule;
}

BOOL ProcessInjection::RemoteFreeLibrary(HANDLE process, HMODULE module)
{
    if (process == nullptr || module == nullptr) return FALSE;

    BOOL result = false;
    HANDLE remotethread = nullptr;

    do
    {
        void *freelibaryptr = GetSysFreeLibaryFuncPtr();
        if (freelibaryptr == nullptr) break;
        remotethread = ::CreateRemoteThread(process, nullptr, 0, (LPTHREAD_START_ROUTINE)freelibaryptr, module, 0, nullptr);
        if (remotethread == nullptr) break;

        ::WaitForSingleObject(remotethread, INFINITE);

        BOOL ret;
        result = ::GetExitCodeThread(remotethread, (unsigned long*)&ret);
        result = ret && result;
    } while(false);

    if (remotethread)
        ::CloseHandle(remotethread);

    return result;
}

BOOL ProcessInjection::RemoteExecute(HANDLE process, HMODULE module, const RemoteAction &action)
{
    if (process == nullptr || module == nullptr || action.funcptr_ == nullptr) return FALSE;

    BOOL result = FALSE;

    void *virmemptr = nullptr;
    HANDLE remotethread = nullptr;

    do
    {
        if (action.param_ && action.paramsize_)
        {
            if (WriteProcessMemory(process, action.param_, action.paramsize_, &virmemptr) == FALSE) break;
        }

        intptr_t realfuncptr = (intptr_t)module - (intptr_t)&__ImageBase + (intptr_t)action.funcptr_;

        remotethread = ::CreateRemoteThread(process, nullptr, 0, (LPTHREAD_START_ROUTINE)realfuncptr, virmemptr, 0, nullptr);
        if (remotethread == nullptr) break;

        ::WaitForSingleObject(remotethread, INFINITE);

        if (virmemptr)
            ::ReadProcessMemory(process, virmemptr, action.param_, action.paramsize_, nullptr);

        BOOL ret;
        result = ::GetExitCodeThread(remotethread, (unsigned long*)&ret);
        result = ret && result;
    } while(false);

    if (virmemptr)
        ::VirtualFreeEx(process, virmemptr, 0, MEM_RELEASE);
    if (remotethread)
        ::CloseHandle(remotethread);

    return result;
}

void *ProcessInjection::GetSysLoadLibaryFuncPtr(void)
{
    HMODULE kernel32 = ::GetModuleHandleW(L"kernel32");
    return ::GetProcAddress(kernel32, "LoadLibraryW");
}

void *ProcessInjection::GetSysFreeLibaryFuncPtr(void)
{
    HMODULE kernel32 = ::GetModuleHandleW(L"kernel32");
    return ::GetProcAddress(kernel32, "FreeLibrary");
}

BOOL ProcessInjection::WriteProcessMemory(HANDLE process, LPCVOID lpaddress, size_t memorysize, void **virmemptraddr)
{
    if (virmemptraddr == nullptr) return FALSE;

    BOOL result = FALSE;
    *virmemptraddr = nullptr;
    void *virmemptr = nullptr;

    do
    {
        virmemptr = ::VirtualAllocEx(process, nullptr, memorysize, MEM_COMMIT, PAGE_READWRITE);
        if (virmemptr == nullptr) break;
        result = ::WriteProcessMemory(process, virmemptr, lpaddress, memorysize, nullptr);
        if (!result) break;
        *virmemptraddr = virmemptr;
    } while(false);

    if (virmemptr && result == FALSE)
        ::VirtualFreeEx(process, virmemptr, 0, MEM_RELEASE);

    return result;
}

HMODULE ProcessInjectionX86::GetProcessModuleHandle(HANDLE thread, const wchar_t *path)
{
    HMODULE module;
    ::GetExitCodeThread(thread, (unsigned long*)&module);
    return module;
}

HMODULE ProcessInjectionX64::GetProcessModuleHandle(HANDLE thread, const wchar_t *path)
{
    HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ::GetProcessIdOfThread(thread));
    MODULEENTRY32W module_entry;
    module_entry.dwSize = sizeof(module_entry);
    if (::Module32FirstW(snapshot, &module_entry))
    {
        do
        {
            if (_wcsicmp(module_entry.szExePath, path) == 0)
            {
                ::CloseHandle(snapshot);
                return (HMODULE)module_entry.modBaseAddr;
            }
        }
        while (::Module32NextW(snapshot, &module_entry));
    }
    ::CloseHandle(snapshot);
    return nullptr;
}