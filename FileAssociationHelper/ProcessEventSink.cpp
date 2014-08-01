#include "ProcessEventSink.h"
#include <stdio.h>

#pragma comment(lib, "wbemuuid.lib")

ULONG ProcessEventSink::AddRef()
{
    return InterlockedIncrement(&ref_);
}

ULONG ProcessEventSink::Release()
{
    LONG lRef = InterlockedDecrement(&ref_);
    if(lRef == 0)
        delete this;
    return lRef;
}

STDMETHODIMP ProcessEventSink::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_IUnknown || riid == IID_IWbemObjectSink)
    {
        *ppv = (IWbemObjectSink *) this;
        AddRef();
        return WBEM_S_NO_ERROR;
    }
    else 
        return E_NOINTERFACE;
}


STDMETHODIMP ProcessEventSink::Indicate(long lObjectCount,
    IWbemClassObject **apObjArray)
{
    for (int index = 0; index < lObjectCount; ++index)
    {
        DWORD processid = 0;
        wchar_t processname[MAX_PATH + 1];
        if (GetProcessInfoFromWbem(apObjArray[index], &processid, processname, MAX_PATH + 1))
        {
            processstartptr_(processid, processname);
        }
    }

    return WBEM_S_NO_ERROR;
}

STDMETHODIMP ProcessEventSink::SetStatus(
            /* [in] */ LONG lFlags,
            /* [in] */ HRESULT hResult,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam
        )
{
    return WBEM_S_NO_ERROR;
}

bool ProcessEventSink::GetProcessInfoFromWbem(IWbemClassObject *apobj, DWORD *processid, wchar_t *processname, size_t processnamelen)
{
    int hitcount = 0;
    SAFEARRAY *pvnames = nullptr;
    apobj->GetNames(nullptr, WBEM_FLAG_ALWAYS | WBEM_MASK_CONDITION_ORIGIN, nullptr, &pvnames);
    long vbl, vbu;
    SafeArrayGetLBound(pvnames, 1, &vbl);
    SafeArrayGetUBound(pvnames, 1, &vbu);
    for(long idx=vbl; idx<=vbu; idx++)
    {
        wchar_t *name = nullptr;
        SafeArrayGetElement(pvnames, &idx, &name);
        if (_wcsicmp(name, L"processid") == 0)
        {
            VARIANT value;
            VariantInit(&value);
            BSTR bstrname = SysAllocString(name);
            if (apobj->Get(bstrname, 0, &value, NULL, 0) == S_OK)
            {
                *processid = value.lVal;
                hitcount++;
            }
            SysFreeString(bstrname);
        }
        else if (_wcsicmp(name, L"processname") == 0)
        {
            VARIANT value;
            VariantInit(&value);
            BSTR bstrname = SysAllocString(name);
            if (apobj->Get(bstrname, 0, &value, NULL, 0) == S_OK)
            {
                wcscpy_s(processname, processnamelen, value.bstrVal);
                hitcount++;
            }
            SysFreeString(bstrname);
        }
    }

    return hitcount == 2 ? true : false;
}