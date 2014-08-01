#pragma once
#include <ShObjIdl.h>

#if defined(_WIN32_WINNT_WIN8)

class CommandExecuteImpl : public IExecuteCommand, public IInitializeCommand, public IObjectWithSelection, public IExecuteCommandApplicationHostEnvironment, public IForegroundTransfer
{
private:
    IExecuteCommand *execmd_;
    bool hookcmd_;
    wchar_t filedire_[MAX_PATH];
    wchar_t filename_[MAX_PATH];

public:
    static bool CreateInstance(IExecuteCommand **execmd);

    // IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, void **object);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

    // IExecuteCommand methods
    STDMETHOD(SetKeyState)(DWORD key_state);
    STDMETHOD(SetParameters)(LPCWSTR params);
    STDMETHOD(SetPosition)(POINT pt);
    STDMETHOD(SetShowWindow)(int show);
    STDMETHOD(SetNoShowUI)(BOOL no_show_ui);
    STDMETHOD(SetDirectory)(LPCWSTR directory);
    STDMETHOD(Execute)(void);

    // IInitializeCommand methods
    STDMETHOD(Initialize)(LPCWSTR name, IPropertyBag* bag);

    // IObjectWithSelection methods
    STDMETHOD(SetSelection)(IShellItemArray* psia);
    STDMETHOD(GetSelection)(REFIID riid, void** ppv);

    // IExecuteCommandApplicationHostEnvironment methods
    STDMETHOD(GetValue)(enum AHE_TYPE* pahe);

    // IForegroundTransfer methods
    STDMETHOD(AllowForegroundTransfer)(void* reserved);

private:
    CommandExecuteImpl(IExecuteCommand *execmd);
    ~CommandExecuteImpl(void);
};

#endif