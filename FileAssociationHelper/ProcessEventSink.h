#pragma once
#include <Wbemidl.h>

class ProcessEventSink : public IWbemObjectSink
{
private:
    LONG ref_;
    typedef void (*ProcessStart)(DWORD processid, const wchar_t *processname);
    ProcessStart processstartptr_;

public:
    ProcessEventSink(ProcessStart processstart) { ref_ = 0; processstartptr_ = processstart; }
    ~ProcessEventSink() { }

    // IUnknown
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // IWbemObjectSink
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
    STDMETHOD(Indicate)( 
            LONG lObjectCount,
            IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray
            );
    STDMETHOD(SetStatus)( 
            /* [in] */ LONG lFlags,
            /* [in] */ HRESULT hResult,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam
            );

private:
    bool GetProcessInfoFromWbem(IWbemClassObject *apobj, DWORD *processid, wchar_t *processname, size_t processnamelen);
};

