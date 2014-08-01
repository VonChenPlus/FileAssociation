#include "FileAssociationHelper.h"


FileAssociationHelperImpl::FileAssociationHelperImpl(void)
{
}

FileAssociationHelperImpl::~FileAssociationHelperImpl(void)
{
}

IUnknown *FileAssociationHelperImpl::GetInterface(const IID &iid)
{
    if (iid == IID_IUnknown)
    {
        return static_cast<IFileAssociationHelper *>(this);
    }
    else if (iid == __uuidof(IFileAssociationHelper))
    {
        return static_cast<IFileAssociationHelper *>(this);
    }

    return nullptr;
}

STDMETHODIMP FileAssociationHelperImpl::OpenHelperDialog(BSTR filepath)
{
    return helperimpl_.OpenHelperDialog(filepath) ? S_OK : S_FALSE;
}

STDMETHODIMP FileAssociationHelperImpl::SearchFromLocal(BSTR filepath)
{
    return helperimpl_.SearchFromLocal(filepath) ? S_OK : S_FALSE;
}

STDMETHODIMP FileAssociationHelperImpl::SearchFromWeb(BSTR fileext)
{
    return helperimpl_.SearchFromWeb(fileext) ? S_OK : S_FALSE;
}