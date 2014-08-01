// FileAssociationService.cpp : Defines the entry point for the application.
//

#include <Windows.h>
#include "FileAssociationService.h"
#include "FileAssociationService_h.h"
#include "../FileAssociationHelperUtils/ComUtils/ServiceRegistration.h"
#include "FileAssociationHelper.h"

volatile long lockcount_ = 0;
HANDLE stopevent_ = nullptr;

long LockService()
{
    return ::InterlockedIncrement(&lockcount_);
}

long UnlockService()
{
    long ret = ::InterlockedDecrement(&lockcount_);
    if (ret == 0)
        ::SetEvent(stopevent_);
    return ret;
}

bool Run()
{
    // register class object
    unsigned long helpertoken = 0;

    do
    {
        IClassFactory *helperfactory;
        if (FAILED(FileAssociationHelper::CreateFactory(&helperfactory))) break;
        HRESULT hr = ::CoRegisterClassObject(__uuidof(FileAssociationHelper), helperfactory, CLSCTX_SERVER, REGCLS_MULTIPLEUSE, &helpertoken);
        helperfactory->Release();
        if (FAILED(hr)) break;

        ::WaitForSingleObject(stopevent_, INFINITE);

    } while (false);

    if (helpertoken)
        ::CoRevokeClassObject(helpertoken);

    return true;
}

bool RunCmdLine(int argc, wchar_t *argv[])
{
    if (argc == 2)
    {
        if (_wcsicmp(argv[1], L"-Embedding") == 0)
        {
            stopevent_ = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
            if (stopevent_ == nullptr)
                return false;
            
            bool ret = Run();

            ::CloseHandle(stopevent_);
        }
        if (_wcsicmp(argv[1], L"/REGSERVER") == 0)
        {
            ServiceRegistration registration;
            return registration.RegisterService();
        }
        if (_wcsicmp(argv[1], L"/UNREGSERVER") == 0)
        {
            ServiceRegistration registration;
            return registration.UnregisterService();
        }
    }
    
    return false;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    HRESULT hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        return 1;
    }

    int argc;
    wchar_t **argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

    bool ret = RunCmdLine(argc, argv);

    ::LocalFree(argv);

    ::CoUninitialize();
    return ret ? 0 : 1;
}