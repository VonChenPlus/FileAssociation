#include "FileAssociationExt.h"
#include <Shlwapi.h>
#include "resource.h"
#include "../FileAssociationService/FileAssociationService_h.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

FileAssociationExtImpl::FileAssociationExtImpl(void)
{
    memset(menuname_, 0, sizeof(wchar_t) * MAX_MENUNAME_LEN);
    ::LoadString((HMODULE)&__ImageBase, CONTEXTMENUNAME, menuname_, MAX_MENUNAME_LEN - 1);
    menuicon_ = LoadBitmap((HMODULE)&__ImageBase, MAKEINTRESOURCE(100));
}

FileAssociationExtImpl::~FileAssociationExtImpl(void)
{
    if (menuicon_)
    {
        DeleteObject(menuicon_);
    }
}

IUnknown *FileAssociationExtImpl::GetInterface(const IID &iid)
{
    if (iid == IID_IUnknown)
    {
        return static_cast<IFileAssociationExt *>(this);
    }
    else if (iid == __uuidof(IFileAssociationExt))
    {
        return static_cast<IFileAssociationExt *>(this);
    }
    else if (iid == __uuidof(IShellExtInit))
    {
        return static_cast<IShellExtInit *>(this);
    }
    else if (iid == __uuidof(IContextMenu))
    {
        return static_cast<IContextMenu *>(this);
    }

    return nullptr;
}

STDMETHODIMP FileAssociationExtImpl::Initialize(PCIDLIST_ABSOLUTE pidlFolder,
            IDataObject *pdtobj,
            HKEY hkeyProgID)
{
    FORMATETC fmt = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM sm = {TYMED_HGLOBAL};
    skipfile_ = true;
    // Get STGMEDIUM data
    HRESULT hr = pdtobj->GetData(&fmt, &sm);
    if (FAILED(hr))
    {
        return hr;
    }

    // Get File Count
    UINT nFileCount = DragQueryFile((HDROP)sm.hGlobal, 0xFFFFFFFF, NULL, 0);
    hr = E_INVALIDARG;
    do
    {
        if (nFileCount < 1) break;
        // Get the FileName
        UINT nLen = DragQueryFile((HDROP)sm.hGlobal, 0, filename_, sizeof(filename_));
        if (nLen <= 0 || nLen >= MAX_PATH) break;
        filename_[nLen] = '\0';
        // Get File Extension
        const wchar_t *ext = PathFindExtension(filename_);
        if (wcslen(ext) != 0)
            skipfile_ = false;
        hr = S_OK;
    } while(false);

    ReleaseStgMedium(&sm);

    return hr;
}

STDMETHODIMP FileAssociationExtImpl::QueryContextMenu( 
            HMENU hmenu,
            UINT indexMenu,
            UINT idCmdFirst,
            UINT idCmdLast,
            UINT uFlags)
{
    if (skipfile_) return E_FAIL;

    if (uFlags & CMF_DEFAULTONLY)
    {
        return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
    }

   

    // Init MenuItemInfo
    MENUITEMINFO mii;
    memset(&mii, 0, sizeof(mii));
    mii.cbSize      = sizeof(mii);
    mii.fMask       = MIIM_STRING | MIIM_CHECKMARKS | MIIM_ID | MIIM_STATE;
    mii.cch         = (UINT)wcslen(menuname_);
    mii.dwTypeData  = menuname_;
    mii.hbmpChecked = menuicon_;
    mii.hbmpUnchecked = menuicon_;
    mii.fState      = MFS_ENABLED;
    mii.wID         = idCmdFirst + indexMenu;
    if (!InsertMenuItem(hmenu, indexMenu, TRUE, &mii))       
    {
        return E_FAIL;
    }

    #define IDM_CTXMENU 0
    // If successful, returns an HRESULT value that has its severity value set to SEVERITY_SUCCESS 
    // and its code value set to the offset of the largest command identifier that was assigned, plus one
    return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, IDM_CTXMENU + 1);
}

STDMETHODIMP FileAssociationExtImpl::InvokeCommand(
            CMINVOKECOMMANDINFO *pici)
{
    do
    {
        HRESULT hr = S_FALSE;
        // execute command
        if(LOWORD(pici->lpVerb) == IDM_CTXMENU)
        {
            const wchar_t *ext = PathFindExtension(filename_);
            if (wcslen(ext) == 0) break;
            // do not need first char
            ext = ext + 1;
            ::CoInitialize(0);
            {
                IFileAssociationHelper *helper;
                hr = ::CoCreateInstance(__uuidof(FileAssociationHelper), nullptr, CLSCTX_LOCAL_SERVER, __uuidof(IFileAssociationHelper), (LPVOID *)&helper);
                if (hr == S_OK)
                {
                    BSTR bstrext = ::SysAllocString(ext);
                    hr = helper->SearchFromWeb(bstrext);
                    ::SysFreeString(bstrext);
                    helper->Release();
                }
            }
            ::CoUninitialize();
            return hr;
        }
    }
    while(false);
    
    return E_FAIL;
}

STDMETHODIMP FileAssociationExtImpl::GetCommandString( 
            UINT_PTR idCmd,
            UINT uType,
            UINT *pReserved,
            LPSTR pszName,
            UINT cchMax)
{
    // do not handle this function
    return S_FALSE;
}