#ifndef PROCESSINJECT_H_
#define PROCESSINJECT_H_

#include <Windows.h>
#include <string>

class ProcessInjection
{
public:
    typedef BOOL (__stdcall *ActionFuncPtr)(void *param);
    struct RemoteAction
    {
        RemoteAction()
        {
            funcptr_ = nullptr;
            param_ = nullptr;
            paramsize_ = 0;
        }
        ActionFuncPtr funcptr_;
        void *param_;
        size_t paramsize_;
    };

    BOOL InjectProcess(DWORD processid, const RemoteAction &action);

private:
    virtual HMODULE GetProcessModuleHandle(HANDLE thread, const wchar_t *path) { return nullptr; }

    HMODULE RemoteLoadLibrary(HANDLE process);
    BOOL RemoteFreeLibrary(HANDLE process, HMODULE module);
    BOOL RemoteExecute(HANDLE process, HMODULE module, const RemoteAction &action);

    void *GetSysLoadLibaryFuncPtr(void);
    void *GetSysFreeLibaryFuncPtr(void);

    BOOL WriteProcessMemory(HANDLE process, LPCVOID lpaddress, size_t memorysize, void **virmemptraddr);
};

class ProcessInjectionX86 : public ProcessInjection
{
private:
    HMODULE GetProcessModuleHandle(HANDLE thread, const wchar_t *path);
};

class ProcessInjectionX64 : public ProcessInjection
{
private:
    HMODULE GetProcessModuleHandle(HANDLE thread, const wchar_t *path);
};

#endif