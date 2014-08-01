#ifndef INJECTIONFACTORY_H_
#define INJECTIONFACTORY_H_

#include <Windows.h>
#include <vector>

class InjectionFactory
{
private:
    typedef bool (WINAPI *BeforeMonitorPtr)();
    typedef BeforeMonitorPtr AfterMonitorPtr;
    typedef BeforeMonitorPtr UninjectTestPtr;
    struct InjectParam
    {
        InjectParam()
        {
            beforeptr_ = nullptr;
            afterptr_ = nullptr;
            uninjecttestptr_ = nullptr;
        }
        InjectParam(const InjectParam &param)
        {
            beforeptr_ = param.beforeptr_;
            afterptr_ = param.afterptr_;
            uninjecttestptr_ = param.uninjecttestptr_;
        }
        BeforeMonitorPtr beforeptr_;
        AfterMonitorPtr afterptr_;
        UninjectTestPtr uninjecttestptr_;
    };

    struct MonitorThreadParam
    {
        MonitorThreadParam()
        {
            injectparam_ = nullptr;
            evt = nullptr;
            ret = false;
        }
        InjectParam *injectparam_;
        HANDLE evt;
        bool ret;
    };

private:
    static UINT shellhookmsg_;

public:
    static bool InjectProcess(const wchar_t *processname, BeforeMonitorPtr, AfterMonitorPtr, UninjectTestPtr);
    static bool UninjectProcess(const wchar_t *processname);
    static bool InjectProcess(DWORD processid, BeforeMonitorPtr, AfterMonitorPtr, UninjectTestPtr);
    static bool UninjectProcess(DWORD processid);

private:
    static BOOL WINAPI RemoteAction(void *);
    static unsigned long WINAPI MonitorThread(void *);

    static std::vector<DWORD> GetProcessIDList(const wchar_t* processname);

    static HWND IsInjected(DWORD processid);
    static void MonitorCircle(void *);
    static LRESULT WINAPI MonitorProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    static bool CreateMonitorWindow(void *);
};

#endif