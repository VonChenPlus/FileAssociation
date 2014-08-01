#include "CommandExecute.h"
#include <new>
#include "../FileAssociationService/FileAssociationService_h.h"

#if defined(_WIN32_WINNT_WIN8)

CommandExecuteImpl::CommandExecuteImpl(IExecuteCommand *execmd)
{
    execmd_ = execmd;
    hookcmd_ = false;
}

CommandExecuteImpl::~CommandExecuteImpl(void)
{
}

bool CommandExecuteImpl::CreateInstance(IExecuteCommand **execmd)
{
    CommandExecuteImpl *instance = new(std::nothrow) CommandExecuteImpl(*execmd);
	if (instance != nullptr)
		*execmd = instance;
    return instance != nullptr;
}

// IUnknown methods

STDMETHODIMP CommandExecuteImpl::QueryInterface(REFIID riid, void **object)
{
	if (riid == IID_IUnknown || riid == IID_IExecuteCommand)
	{
		*object = static_cast<IExecuteCommand*>(this);
	}
	else if (riid == IID_IInitializeCommand)
	{
		*object = static_cast<IInitializeCommand*>(this);
	}
	else if (riid == IID_IObjectWithSelection)
	{
		*object = static_cast<IObjectWithSelection*>(this);
	}
	else if (riid == IID_IExecuteCommandApplicationHostEnvironment)
	{
		*object = static_cast<IExecuteCommandApplicationHostEnvironment*>(this);
	}
    else if (riid == IID_IForegroundTransfer)
	{
		*object = static_cast<IForegroundTransfer*>(this);
	}
    else
    {
        return E_NOINTERFACE;
    }

	execmd_->AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) CommandExecuteImpl::AddRef()
{
    return execmd_->AddRef();
}

STDMETHODIMP_(ULONG) CommandExecuteImpl::Release()
{
    ULONG l = execmd_->Release();
    if (l == 0)
    {
        delete this;
    }

    return l;
}

// IExecuteCommand methods
STDMETHODIMP CommandExecuteImpl::SetKeyState(DWORD key_state)
{
    return execmd_->SetKeyState(key_state);
}

STDMETHODIMP CommandExecuteImpl::SetParameters(LPCWSTR params)
{
    return execmd_->SetParameters(params);
}

STDMETHODIMP CommandExecuteImpl::SetPosition(POINT pt)
{
    return execmd_->SetPosition(pt);
}

STDMETHODIMP CommandExecuteImpl::SetShowWindow(int show)
{
    return execmd_->SetShowWindow(show);
}

STDMETHODIMP CommandExecuteImpl::SetNoShowUI(BOOL no_show_ui)
{
    return execmd_->SetNoShowUI(no_show_ui);
}

STDMETHODIMP CommandExecuteImpl::SetDirectory(LPCWSTR directory)
{
    wcscpy_s(filedire_, directory);
    return execmd_->SetDirectory(directory);
}

STDMETHODIMP CommandExecuteImpl::Execute()
{
    if (hookcmd_)
    {
        HRESULT hr = ::CoInitialize(0);
        {
            IFileAssociationHelper *helper;
            hr = ::CoCreateInstance(__uuidof(FileAssociationHelper), nullptr, CLSCTX_LOCAL_SERVER, __uuidof(IFileAssociationHelper), (LPVOID *)&helper);
            if (SUCCEEDED(hr))
            {
                wchar_t filepath[MAX_PATH + 1] = {0};
                wsprintf(filepath, L"%s%s", filedire_, filename_);
                BSTR bstrfilename = ::SysAllocString(filepath);
                hr = helper->OpenHelperDialog(bstrfilename);
                ::SysFreeString(bstrfilename);
                helper->Release();
            }
        }
        ::CoUninitialize();

        return hr;
    }
    
    return execmd_->Execute();
}

// IInitializeCommand methods
STDMETHODIMP CommandExecuteImpl::Initialize(LPCWSTR name, IPropertyBag* bag)
{
    IInitializeCommand *initcmd;
	HRESULT hr = execmd_->QueryInterface(&initcmd);
	if (hr != S_OK)
		return hr;
    hr = initcmd->Initialize(name, nullptr);
	initcmd->Release();

    hookcmd_ = _wcsicmp(name, L"openas") == 0;

    return hr;
}

// IObjectWithSelection methods
STDMETHODIMP CommandExecuteImpl::SetSelection(IShellItemArray* psia)
{
    IObjectWithSelection *selection;
	HRESULT hr = execmd_->QueryInterface(&selection);
	if (hr != S_OK)
		return hr;
	DWORD count;
	psia->GetCount(&count);
	for (DWORD index = 0; index < count && count == 1; ++index)
	{
		IShellItem *psi;
		psia->GetItemAt(index, &psi);
		LPWSTR pszName;
		psi->GetDisplayName(SIGDN_NORMALDISPLAY, &pszName);
        wcscpy_s(filename_, pszName);
	}
	hr = selection->SetSelection(psia);
	selection->Release();
	return hr;
}

STDMETHODIMP CommandExecuteImpl::GetSelection(REFIID riid, void** ppv)
{
    IObjectWithSelection *selection;
    HRESULT hr = execmd_->QueryInterface(&selection);
	if (hr != S_OK)
		return hr;
	hr = selection->GetSelection(riid, ppv);
	selection->Release();
    return hr;
}

// IExecuteCommandApplicationHostEnvironment methods
STDMETHODIMP CommandExecuteImpl::GetValue(enum AHE_TYPE* pahe)
{
    IExecuteCommandApplicationHostEnvironment *execmdenv;
    HRESULT hr = execmd_->QueryInterface(&execmdenv);
    if (hr != S_OK)
        return hr;
    hr = execmdenv->GetValue(pahe);
    execmdenv->Release();
    return hr;
}

// IForegroundTransfer methods
STDMETHODIMP CommandExecuteImpl::AllowForegroundTransfer(void* reserved)
{
    IForegroundTransfer *foregtran;
    HRESULT hr = execmd_->QueryInterface(&foregtran);
    if (hr != S_OK)
        return hr;
    hr = foregtran->AllowForegroundTransfer(reserved);
    foregtran->Release();
    return hr;
}

#endif