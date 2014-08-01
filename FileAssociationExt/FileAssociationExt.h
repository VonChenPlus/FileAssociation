#pragma once
#include "FileAssociationExt_h.h"
#include "../FileAssociationHelperUtils/ComUtils/ComFactory.h"
#include <Shobjidl.h>

class FileAssociationExtImpl : public IFileAssociationExt, public IShellExtInit, public IContextMenu
{
private:
    #define MAX_MENUNAME_LEN 256
    WCHAR menuname_[MAX_MENUNAME_LEN];
    WCHAR filename_[MAX_PATH];
    HBITMAP menuicon_;
    bool skipfile_;

protected:
    FileAssociationExtImpl(void);
    ~FileAssociationExtImpl(void);

    IUnknown *GetInterface(const IID &iid);

    // IShellExtInit methods
    STDMETHOD(Initialize)(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);

    // IContextMenu methods
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(CMINVOKECOMMANDINFO *pici);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pReserved, LPSTR pszName, UINT cchMax);
};

class FileAssociationExt : public CComObject<FileAssociationExtImpl>
{
public:
    DECLARE_COMFACTORY(FileAssociationExt)
};