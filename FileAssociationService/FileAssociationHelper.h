#pragma once
#include "FileAssociationService_h.h"
#include "../FileAssociationHelperUtils/ComUtils/ComFactory.h"
#include "HelperImpl.h"

class FileAssociationHelperImpl : public IFileAssociationHelper
{
private:
    HelperImpl helperimpl_;

protected:
    FileAssociationHelperImpl(void);
    ~FileAssociationHelperImpl(void);

    IUnknown *GetInterface(const IID &iid);

public:
    STDMETHOD(OpenHelperDialog)(BSTR filepath);
    STDMETHOD(SearchFromLocal)(BSTR filepath);
    STDMETHOD(SearchFromWeb)(BSTR fileext);
};

class FileAssociationHelper : public CComObject<FileAssociationHelperImpl>
{
public:
    DECLARE_COMFACTORY(FileAssociationHelper)
};