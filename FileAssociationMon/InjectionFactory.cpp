#include "InjectionFactory.h"
#include <TlHelp32.h>
#include "ProcessInjection.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

#define WM_UNINJECT WM_USER + 100

#define FILEASSOCIATIONMONCLASS L"FileAssociationMonClass"
#define FILEASSOCIATIONMONWINDOW L"FileAssociationMonWindow"

UINT InjectionFactory::shellhookmsg_ = 0;

bool InjectionFactory::InjectProcess(const wchar_t *processname, BeforeMonitorPtr beforeptr, AfterMonitorPtr afterptr, UninjectTestPtr uninjecttestptr)
{
    if (processname == nullptr || wcslen(processname) == 0) return false;

    bool ret = false;
    std::vector<DWORD> idlist = GetProcessIDList(processname);
    for (std::vector<DWORD>::iterator it = idlist.begin(); it != idlist.end(); ++it)
    {
        ret |= InjectProcess((*it), beforeptr, afterptr, uninjecttestptr);
    }
    return ret;
}

bool InjectionFactory::UninjectProcess(const wchar_t *processname)
{
    if (processname == nullptr) return false;

    std::vector<DWORD> idlist = GetProcessIDList(processname);
    for (std::vector<DWORD>::iterator it = idlist.begin(); it != idlist.end(); ++it)
    {
        UninjectProcess((*it));
    }

    return true;
}

bool InjectionFactory::InjectProcess(DWORD processid, BeforeMonitorPtr beforeptr, AfterMonitorPtr afterptr, UninjectTestPtr uninjecttestptr)
{
    if (IsInjected(processid) != nullptr)
        return false;
    ProcessInjection::RemoteAction action;
    action.funcptr_ = InjectionFactory::RemoteAction;
    InjectParam injectparam;
    injectparam.beforeptr_ = beforeptr;
    injectparam.afterptr_ = afterptr;
    injectparam.uninjecttestptr_ = uninjecttestptr;
    action.param_ = &injectparam;
    action.paramsize_ = sizeof(injectparam);
#ifdef _M_IX86
    ProcessInjectionX86 injectionx86;
    return injectionx86.InjectProcess(processid, action) == TRUE ? true : false;
#elif defined _M_AMD64
    ProcessInjectionX64 injectionx64;
    return injectionx64.InjectProcess(processid, action) == TRUE ? true : false;
#else
#error Unsupported CPU for remote code inject 
#endif
}

bool InjectionFactory::UninjectProcess(DWORD processid)
{
    HWND hwnd = IsInjected(processid);
    if (hwnd != nullptr)
    {
        return ::SendMessageW(hwnd, WM_UNINJECT, 0, 0) == TRUE ? true : false;
    }
    return false;
}

BOOL WINAPI InjectionFactory::RemoteAction(void *virmemptr)
{
    MonitorThreadParam param;
    BOOL result = FALSE;
    do
    {
        param.evt = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (param.evt == nullptr) break;
        param.injectparam_ = (InjectParam *)virmemptr;
        HANDLE thread = ::CreateThread(nullptr, 0, InjectionFactory::MonitorThread, &param, 0, nullptr);
        if (thread == nullptr) break;

        ::WaitForSingleObject(param.evt, INFINITE);

        if (!param.ret)
            ::WaitForSingleObject(thread, INFINITE);
        ::CloseHandle(thread);

        result = TRUE;
    } while(false);

    if (param.evt)
        ::CloseHandle(param.evt);

    return result;
}

unsigned long WINAPI InjectionFactory::MonitorThread(void *paramaddr)
{
    MonitorThreadParam *param = (MonitorThreadParam *)paramaddr;
    if (param == nullptr || param->injectparam_ == nullptr) return FALSE;

    HANDLE evt = param->evt;
    bool result = false;
    HMODULE module = nullptr;
    InjectParam injectparam(*(param->injectparam_));

    do
    {
        BOOL ret = ::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (const wchar_t*)&__ImageBase, &module);
        result = ret == TRUE;

        if (injectparam.beforeptr_)
        {
            intptr_t realfuncptr = (intptr_t)module - (intptr_t)&__ImageBase + (intptr_t)injectparam.beforeptr_;
            result = result && ((BeforeMonitorPtr)realfuncptr)();
        }
        
        param->ret = result;
        ::SetEvent(evt);

        if (result)
        {
            MonitorCircle(&injectparam);
        }
    } while(false);

    if (module)
    {
        if (injectparam.afterptr_)
        {
            intptr_t realfuncptr = (intptr_t)module - (intptr_t)&__ImageBase + (intptr_t)injectparam.afterptr_;
            ((AfterMonitorPtr)realfuncptr)();
        }
        
        while (true)
        {
            intptr_t realfuncptr = (intptr_t)module - (intptr_t)&__ImageBase + (intptr_t)injectparam.uninjecttestptr_;
            if (!((UninjectTestPtr)realfuncptr)()) continue;
            ::FreeLibraryAndExitThread(module, 0);
        }
    }

    return result ? TRUE : FALSE;
}

std::vector<DWORD> InjectionFactory::GetProcessIDList(const wchar_t* processname)
{
    PROCESSENTRY32 processentry32;
    HANDLE   handlesnapshot;
    std::vector<DWORD> processlist;
    BOOL flag;

    RtlZeroMemory(&processentry32, sizeof(processentry32));
    processentry32.dwSize = sizeof(processentry32);
    handlesnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    flag = Process32First(handlesnapshot, &processentry32);
    while ( flag )
    {
        HANDLE hprocess = ::OpenProcess(PROCESS_QUERY_INFORMATION , FALSE, processentry32.th32ProcessID);
        if(hprocess != nullptr)
        {
            if(_wcsicmp(processentry32.szExeFile, processname) == 0)
            {
                processlist.push_back(processentry32.th32ProcessID);
            }
            ::CloseHandle(hprocess);
        }
        flag = Process32Next(handlesnapshot, &processentry32);
    }       
    CloseHandle(handlesnapshot);
    return processlist;
}

HWND InjectionFactory::IsInjected(DWORD processid)
{
    HWND hwnd = nullptr;
    while ((hwnd = ::FindWindowExW(HWND_MESSAGE, hwnd, FILEASSOCIATIONMONCLASS, FILEASSOCIATIONMONWINDOW)) != nullptr)
    {
        DWORD id;
        DWORD threadid = ::GetWindowThreadProcessId(hwnd, &id);
        if (threadid != 0)
        {
            if(id == processid)
                return hwnd;
        }
    }
    return nullptr;
}

void InjectionFactory::MonitorCircle(void *param)
{
    if (!CreateMonitorWindow(param))
        return;

    MSG msg;
    BOOL ret;
    while ((ret = ::GetMessageW(&msg, nullptr, 0, 0)) != 0)
    {
        if (ret != -1)
        {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
    }

    ::UnregisterClassW(FILEASSOCIATIONMONCLASS, (HMODULE)&__ImageBase);
}

LRESULT WINAPI InjectionFactory::MonitorProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_UNINJECT:
        {
            InjectParam *injectparam = (InjectParam *)::GetWindowLongPtrW(hwnd, GWLP_USERDATA);
            HMODULE module = nullptr;
            if (injectparam->uninjecttestptr_ 
                && ::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (const wchar_t*)&__ImageBase, &module) == TRUE)
            {
                intptr_t realfuncptr = (intptr_t)module - (intptr_t)&__ImageBase + (intptr_t)injectparam->uninjecttestptr_;
                ::FreeLibrary(module);
                if (((UninjectTestPtr)realfuncptr)())
                {
                    ::SendMessageW(hwnd, WM_CLOSE, 0, 0);
                    return TRUE;
                }
            }
            return FALSE;
        }
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }

    return ::DefWindowProcW(hwnd, msg, wparam, lparam);
}

bool InjectionFactory::CreateMonitorWindow(void *param)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(wcex);
    wcex.style = 0;
    wcex.lpfnWndProc = MonitorProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = (HMODULE)&__ImageBase;
    wcex.hIcon = nullptr;
    wcex.hCursor = nullptr;
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = FILEASSOCIATIONMONCLASS;
    wcex.hIconSm = nullptr;

    ATOM atom = ::RegisterClassExW(&wcex);
    if (atom == 0)
    {
        return false;
    }

    HWND hwnd = nullptr;
    do
    {
        hwnd = ::CreateWindowExW(0, FILEASSOCIATIONMONCLASS, FILEASSOCIATIONMONWINDOW, WS_POPUP, 0, 0, 0, 0, HWND_MESSAGE, nullptr, (HMODULE)&__ImageBase, nullptr);
        if (hwnd == nullptr) break;

        shellhookmsg_ = RegisterWindowMessage(L"SHELLHOOK");
        if (RegisterShellHookWindow(hwnd) == FALSE) break;
        ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)param);

        return true;
    } while(false);

    if (hwnd)
        ::UnregisterClassW(FILEASSOCIATIONMONCLASS, (HMODULE)&__ImageBase);

    return false;
}