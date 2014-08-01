#include "HelperImpl.h"
#include <Windows.h>
#include "HelperDialog.h"

#define SEARCHWEB_CMD_FORMAT L"http://www.openthefile.com/ext/%s?utm_source=fah&utm_medium=searchweb&utm_campaign=fah"
#define SEARCHLOCAL_CMD_FORMAT L"rundll32.exe shell32.dll,OpenAs_RunDLL %s  "

HelperImpl::HelperImpl(void)
{
}


HelperImpl::~HelperImpl(void)
{
}

bool HelperImpl::OpenHelperDialog(const wchar_t *filepath)
{
    if (filepath == nullptr || wcslen(filepath) == 0) return false;

    HelperDialog dialog;
    return dialog.DoMoadl(nullptr, filepath);
}

bool HelperImpl::SearchFromLocal(const wchar_t *filepath)
{
    if (filepath == nullptr || wcslen(filepath) == 0) return false;

    size_t cmdlen = wcslen(SEARCHLOCAL_CMD_FORMAT) + wcslen(filepath) + 1;
    wchar_t *cmd = new wchar_t[cmdlen];
    swprintf_s(cmd, cmdlen, SEARCHLOCAL_CMD_FORMAT L"\0", filepath);

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    BOOL ret = ::CreateProcessW(L"rundll32.exe", cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    if(ret)
    {
        ::CloseHandle( pi.hProcess );
        ::CloseHandle( pi.hThread );
    }

    delete[] cmd;

    return ret == TRUE ? true : false;
}

bool HelperImpl::SearchFromWeb(const wchar_t *fileext)
{
    if (fileext == nullptr) return false;
    wchar_t *formatext = nullptr;
    FormatString(fileext, &formatext);
    if (formatext == nullptr) return false;

    size_t urllen = wcslen(SEARCHWEB_CMD_FORMAT) + wcslen(formatext) + 1;
    wchar_t *url = new wchar_t[urllen];
    swprintf_s(url, urllen, SEARCHWEB_CMD_FORMAT L"\0", formatext);
    ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);

    delete[] formatext;
    delete[] url;

    return true;
}

void HelperImpl::FormatString(const wchar_t *src, wchar_t **dest)
{
    *dest = nullptr;
    size_t srclen = 0;
    if (src == nullptr || (srclen = wcslen(src)) == 0) return;

    // pre-alloc
    size_t destlen = srclen * 2;
    *dest = new wchar_t[destlen];
    memset(*dest, 0, sizeof(wchar_t) * destlen);

    size_t curlen = 0;
    for (size_t index = 0; index < srclen;)
    {
        WCHAR ch = src[index];
        
        if (ch == '\0') break;
        if (ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch >= '0' && ch <= '9' ||
            ch == '-' || ch == '_' || ch == '.' || ch == '*')
        {
            if (curlen + 1 < destlen - 1)
            {
                (*dest)[curlen] = ch;
                curlen += 1;
                ++index;
                continue;
            }
        }
        else
        {
            size_t bufindex = sizeof(wchar_t) * 2;
            if (curlen + bufindex + 1 < destlen - 1)
            {
                // add '%' and '/0'
                wchar_t tempbuf[sizeof(wchar_t) * 2 + 2] = {0};
                while (ch)
                {
                    tempbuf[bufindex--] = L"0123456789abcdef"[(ch & 0x0f)];
                    tempbuf[bufindex--] = L"0123456789abcdef"[(ch & 0xf0) >> 4];
                    ch >>= 8;
                }
                tempbuf[bufindex--] = '%';
                wcscpy_s(*dest + curlen, destlen - curlen, tempbuf);
                curlen += wcslen(tempbuf);
                ++index;
                continue;
            }
        }

        //re-alloc
        destlen *= 2;
        wchar_t *tempbuf = new wchar_t[destlen];
        memset(tempbuf, 0, sizeof(wchar_t) * destlen);
        wcscpy_s(tempbuf, destlen, *dest);
        delete[] *dest;
        *dest = tempbuf;
    }
}