// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>
#include "FileAssociationExt_h.h"
#include "FileAssociationExt.h"
#include "../FileAssociationHelperUtils/ComUtils/ServiceRegistration.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

volatile long lockcount_ = 0;

long LockService()
{
    return ::InterlockedIncrement(&lockcount_);
}

long UnlockService()
{
    return ::InterlockedDecrement(&lockcount_);
}

// Used to determine whether the DLL can be unloaded by OLE.
STDAPI DllCanUnloadNow(void)
{
    if (lockcount_ == 0)
        return S_OK;

    return S_FALSE;
}

// Returns a class factory to create an object of the requested type.
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    if (ppv == NULL)
    {
        return E_POINTER;
    }

    *ppv = NULL;

    HRESULT hr = S_OK;

    if (rclsid == __uuidof(FileAssociationExt))
    {
        if (riid == __uuidof(IClassFactory))
        {
            IClassFactory *factory = NULL;
            FileAssociationExt::CreateFactory(&factory);
            factory->QueryInterface(IID_IUnknown, ppv);
            factory->Release();
        }
    }

    if (*ppv == NULL)
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }

    return hr;
}

// DllRegisterServer - Adds entries to the system registry.
STDAPI DllRegisterServer(void)
{
    ::CoInitialize(NULL);
    ServiceRegistration registration;
    bool ret = registration.RegisterService();
    ::CoUninitialize();
    return ret ? S_OK : S_FALSE;
}

// DllUnregisterServer - Removes entries from the system registry.
STDAPI DllUnregisterServer(void)
{
    ::CoInitialize(NULL);
    ServiceRegistration registration;
    bool ret = registration.UnregisterService();
    ::CoUninitialize();
    return ret ? S_OK : S_FALSE;
}