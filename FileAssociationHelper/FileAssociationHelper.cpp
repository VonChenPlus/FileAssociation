// FileAssociationHelper.cpp : Defines the entry point for the application.
//

#include <Windows.h>
#include <Shlwapi.h>
#include <Psapi.h>
#include "FileAssociationHelper.h"
#include "ProcessEventSink.h"
#include <comdef.h>
#include "../FileAssociationMon/ExplorerInjection.h"

#pragma comment(lib, "Shlwapi.lib")

extern "C" IMAGE_DOS_HEADER __ImageBase;

#define FILEASSOCIATEHELPERCLASS L"FileAssociateHelperClass"
#define FILEASSOCIATEHELPERWINDOW L"FileAssociateHelperWindow"

HANDLE FileAssociationHelper::stopevent_ = nullptr;

bool RunCmdLine(int argc, wchar_t *argv[])
{
    if (argc == 2)
    {
        if (_wcsicmp(argv[1], L"/START") == 0)
        {
            return FileAssociationHelper::RunStartCmd();
        }
        if (_wcsicmp(argv[1], L"/STOP") == 0)
        {
            HANDLE m_hMutex = ::CreateEventW(nullptr, TRUE, FALSE, L"FileAssociateHelper Mutex");
            return FileAssociationHelper::RunStopCmd();
        }
    }

    return false;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    int argc;
    wchar_t **argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    
    bool ret = RunCmdLine(argc, argv);

    ::LocalFree(argv);

    return ret ? 0 : 1;
}

UINT FileAssociationHelper::shellhookmsg_ = 0;

bool FileAssociationHelper::RunStartCmd()
{
    // Make sure only one instance exists
    stopevent_ = ::CreateEventW(nullptr, TRUE, FALSE, L"FileAssociateHelper Event");
    if (GetLastError() == ERROR_ALREADY_EXISTS) return false;

    // Injection all Explorer Process
    if (!ExplorerInjection::InjectProcess()) return false;

    // Monitor Process Start
    CreateProcessTrack();

    return true;
}

bool FileAssociationHelper::RunStopCmd()
{
    stopevent_ = ::CreateEventW(nullptr, TRUE, FALSE, L"FileAssociateHelper Event");
    if (stopevent_ == nullptr) return false;
    ::SetEvent(stopevent_);

    // Uninjection all Explorer Process
    return ExplorerInjection::UninjectProcess();
}

HRESULT FileAssociationHelper::CreateProcessTrack()
{
    HRESULT hr;
    IWbemLocator *pLoc = nullptr;
    IWbemServices *pSvc = nullptr;
    IUnsecuredApartment* pUnsecApp = nullptr;
    ProcessEventSink* pSink = nullptr;
    IUnknown* pStubUnk = nullptr;
    IWbemObjectSink* pStubSink = nullptr;
    do
    {
        hr = CoInitializeEx(0, COINIT_MULTITHREADED); 
        if (FAILED(hr)) break;
        hr = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
        if (FAILED(hr)) break;
        hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pLoc);
        if (FAILED(hr)) break;
        hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
        if (FAILED(hr)) break;
        hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
        if (FAILED(hr)) break;
        hr = CoCreateInstance(CLSID_UnsecuredApartment, nullptr, CLSCTX_LOCAL_SERVER, IID_IUnsecuredApartment, (void**)&pUnsecApp);
        pSink = new ProcessEventSink(FileAssociationHelper::ProcessStartCallback);
        pSink->AddRef();
        hr = pUnsecApp->CreateObjectStub(pSink, &pStubUnk);
        if (FAILED(hr)) break;
        hr = pStubUnk->QueryInterface(IID_IWbemObjectSink, (void **) &pStubSink);
        if (FAILED(hr)) break;
        hr = pSvc->ExecNotificationQueryAsync(_bstr_t("WQL"), _bstr_t("SELECT * FROM Win32_ProcessStartTrace"), WBEM_FLAG_SEND_STATUS, nullptr, pStubSink);
    } while (false);

    WaitForSingleObject(stopevent_, INFINITE);

    if (pSvc)
    {
        if (pStubSink)
        {
            pSvc->CancelAsyncCall(pStubSink);
            pStubSink->Release();
        }
        pSvc->Release();
    }
    if (pLoc)
        pLoc->Release();
    if (pUnsecApp)
        pUnsecApp->Release();
    if (pStubUnk)
        pStubUnk->Release();
    if (pSink)
        pSink->Release();
    
    CoUninitialize();

    return hr;
}

void FileAssociationHelper::ProcessStartCallback(DWORD processid, const wchar_t *processname)
{
    HANDLE hprocess = ::OpenProcess(PROCESS_QUERY_INFORMATION , FALSE, processid);
    if(hprocess != nullptr)
    {
        WCHAR processpath[MAX_PATH + 1];
        ::GetProcessImageFileName(hprocess, processpath, MAX_PATH);
        const wchar_t *processname = PathFindFileName(processpath);
        if(_wcsicmp(ExplorerInjection::processname_, processname) == 0)
        {
            ExplorerInjection::InjectProcess(processid);
        }
        ::CloseHandle(hprocess);
    }
}