#include "ServiceRegistration.h"
#include <atlbase.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;

ServiceRegistration::ServiceRegistration(void)
{
}

ServiceRegistration::~ServiceRegistration(void)
{
}

bool ServiceRegistration::RegisterService()
{
    if (!RegisterTLB())
        return false;

    if (!RegisterResource(100))
    {
        RegisterTLB(false);
        return false;
    }

    return true;
}

bool ServiceRegistration::UnregisterService()
{
    return RegisterResource(100, false) && RegisterTLB(false);
}

bool ServiceRegistration::RegisterTLB(bool regist)
{
    wchar_t tlbname[MAX_PATH];
    ::GetModuleFileNameW((HMODULE)&__ImageBase, tlbname, MAX_PATH);

    ITypeLib *tlb;
    if (FAILED(::LoadTypeLibEx(tlbname, REGKIND_REGISTER, &tlb)))
    {
        return false;
    }

    if (!regist)
    {
        TLIBATTR *ptla;
        HRESULT hr = tlb->GetLibAttr(&ptla);
        if (FAILED(hr))
        {
            return false;
        }

        hr = ::UnRegisterTypeLib(ptla->guid, ptla->wMajorVerNum, ptla->wMinorVerNum, ptla->lcid, ptla->syskind);

        tlb->ReleaseTLibAttr(ptla);
    }

    tlb->Release();
    return true;
}

bool ServiceRegistration::RegisterResource(int resid, bool regist)
{
    IRegistrar *reg;
    HRESULT hr = ::CoCreateInstance(CLSID_Registrar,
                                    nullptr,
                                    CLSCTX_INPROC_SERVER,
                                    IID_IRegistrar,
                                    reinterpret_cast<void**>(&reg));
    if (FAILED(hr))
    {
        return false;
    }

    wchar_t modulename[MAX_PATH];
    ::GetModuleFileNameW((HMODULE)&__ImageBase, modulename, MAX_PATH);
    hr = reg->AddReplacement(L"MODULE", modulename);
    if (SUCCEEDED(hr))
    {
        if (regist)
        {
            hr = reg->ResourceRegister(modulename, resid, L"REGISTRY");
        }
        else
        {
            hr = reg->ResourceUnregister(modulename, resid, L"REGISTRY");
        }
    }
    reg->Release();
    return SUCCEEDED(hr);
}