#pragma once
#include <Windows.h>
#include <new>

extern long LockService();
extern long UnlockService();

class ServiceLock
{
public:
    static void Lock() throw()
    {
        LockService();
    }

    static void Unlock() throw()
    {
        UnlockService();
    }
};

class NoServiceLock
{
public:
    static void Lock() throw()
    {
    }

    static void Unlock() throw()
    {
    }
};

template<typename Impl, typename Lock = ServiceLock>
class CComObject : public Impl
{
private:
    long ref_;

protected:
    CComObject() throw() : ref_(1)
    {
        Lock::Lock();
    }

    ~CComObject() throw()
    {
        Lock::Unlock();
    }

public:
    STDMETHODIMP QueryInterface(const IID &iid, void** ppv)
    {
        if (ppv == nullptr)
        {
            return E_POINTER;
        }

        *ppv = reinterpret_cast<void**>(Impl::GetInterface(iid));

        if (*ppv == nullptr)
        {
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        return ::InterlockedIncrement(&ref_);;
    }

    STDMETHODIMP_(ULONG) Release()
    {
        ULONG l = ::InterlockedDecrement(&ref_);
        if (l == 0)
        {
            delete this;
        }

        return l;
    }

    template<typename I>
    static HRESULT CreateInstance(I **object) throw()
    {
        if (object == nullptr)
        {
            return E_POINTER;
        }

        *object = static_cast<I*>(new(std::nothrow) CComObject);
        if (object == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }
};