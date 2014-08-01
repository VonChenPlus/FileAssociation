#pragma once
#include "ComObject.h"

template<typename ComObject>
class ComFactoryImpl : public IClassFactory
{
protected:
    IUnknown *GetInterface(const IID &iid)
    {
        if (iid == IID_IClassFactory || iid == IID_IUnknown || iid == IID_IDispatch)
        {
            return static_cast<IUnknown *>(this);
        }

        return nullptr;
    }

public:
    STDMETHOD(CreateInstance)(IUnknown *outer, const IID &iid, void **ppv)
    {
        if (outer != nullptr)
        {
            return CLASS_E_NOAGGREGATION;
        }
        if (ppv == nullptr)
        {
            return E_POINTER;
        }

        if (!LockService())
        {
            return CO_E_SERVER_STOPPING;
        }

        ComObject *object;
        HRESULT hr = ComObject::CreateInstance(&object);
        if (SUCCEEDED(hr))
        {
            hr = object->QueryInterface(iid, ppv);
            object->Release();
        }

        UnlockService();

        return hr;
    }

    STDMETHOD(LockServer)(BOOL lock)
    {
        if (lock)
            LockService();
        else
            UnlockService();

        return S_OK;
    }
};

#ifndef DECLARE_COMFACTORY
#define DECLARE_COMFACTORY(classname)\
    typedef CComObject<ComFactoryImpl<classname>, NoServiceLock> classname##Factory;\
    template<typename I>\
    static HRESULT CreateFactory(I **object)\
    {\
        return classname##Factory::CreateInstance(object);\
    }
#endif