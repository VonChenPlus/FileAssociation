#include "HelperDialog.h"
#include <CommCtrl.h>
#include "resource.h"
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Comctl32.lib")

extern "C" IMAGE_DOS_HEADER __ImageBase;

#define REG_FAH_EXT_KEY L"SOFTWARE\\Nico Mak Computing\\File Association Helper\\EXTS"

HelperDialog::HelperDialog(void)
{
    parenthwnd_ = nullptr;
    helperaction_ = SEARCHFROMWEB;
    filename_ = nullptr;
    fileext_ = nullptr;
}

HelperDialog::~HelperDialog(void)
{
    for (std::vector<wchar_t *>::iterator it = excludeextlist_.begin(); it != excludeextlist_.end(); ++it)
    {
        if (*it)
            delete[] (*it);
    }
    excludeextlist_.clear();
}

bool HelperDialog::DoMoadl(HWND parenthwnd, const wchar_t *filepath)
{
    if (filepath == nullptr || wcslen(filepath) == 0) return false;

    parenthwnd_ = parenthwnd;
    wcscpy_s(filepath_, filepath);
    filename_ = PathFindFileNameW(filepath_);
    if (wcslen(filename_) == 0) return false;
    fileext_ = PathFindExtensionW(filepath_);
    if (wcslen(fileext_) == 0) return false;
    // do not need '.'
    fileext_++;

    InitCommonControls();
    DialogBoxParamW((HMODULE)&__ImageBase, MAKEINTRESOURCEW(IDD_HELPER_DIALOG), parenthwnd_, WndProc, (LPARAM)this);
    return true;
}

void HelperDialog::SetDialogData(HWND hwnd, int index, HelperDialog *data)
{
    ::SetWindowLongPtrW(hwnd, index, (LONG_PTR)data);
}

HelperDialog *HelperDialog::GetDialogData(HWND hwnd, int index)
{
    return (HelperDialog *)::GetWindowLongPtrW(hwnd, index);
}

INT_PTR CALLBACK HelperDialog::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            HelperDialog *data = (HelperDialog *)lparam;
            if (data == nullptr) break;

            data->selfhwnd_ = hwnd;
            SetDialogData(hwnd, GWLP_USERDATA, data);

            return data->OnInitDialog();
        }
        break;
    case WM_COMMAND:
        {
            HelperDialog *data = GetDialogData(hwnd, GWLP_USERDATA);
            if (data)
            {
                return data->OnCommand((int)wparam);
            }
        }
    case WM_CLOSE:
        {
            return (INT_PTR)EndDialog(hwnd, 0);
        }
    }

    return (INT_PTR)FALSE;
}

INT_PTR HelperDialog::OnInitDialog()
{
    if (!LoadExcludedDataFromReg(excludeextlist_)) return (INT_PTR)FALSE;
    if (IsExcluded(fileext_))
    {
        return helperimpl_.SearchFromLocal(filepath_) ? (INT_PTR)TRUE : (INT_PTR)FALSE; 
    }
    
    //set default radio button clicked
    HWND hwnd = GetDlgItem(selfhwnd_, IDC_SEARCH_WEB_RADIO);
    SendMessageW(hwnd, BM_SETCHECK, BST_CHECKED, 0);

    //set window static text
    hwnd = GetDlgItem(selfhwnd_, IDC_FILE_NAME);
    SetWindowTextW(hwnd, filename_);
    hwnd = GetDlgItem(selfhwnd_, IDC_LEARN_MORE_RADIO);
    wchar_t *formatbuf = nullptr;
    LoadStringFromRes(LEARN_MORE_FORMAT, &formatbuf);
    if (formatbuf != nullptr)
    {
        size_t tempbufsize = wcslen(formatbuf) + wcslen(fileext_);
        wchar_t *tempbuf = new wchar_t[tempbufsize];
        wsprintfW(tempbuf, formatbuf, fileext_);
        SetWindowTextW(hwnd, tempbuf);
        delete tempbuf;
        delete formatbuf;
        formatbuf = nullptr;
    }
    hwnd = GetDlgItem(selfhwnd_, IDC_NOT_SHOW_CHECK);
    LoadStringFromRes(CHECK_BOX_FORMAT, &formatbuf);
    if (formatbuf != nullptr)
    {
        size_t tempbufsize = wcslen(formatbuf) + wcslen(fileext_);
        wchar_t *tempbuf = new wchar_t[tempbufsize];
        wsprintfW(tempbuf, formatbuf, fileext_);
        SetWindowTextW(hwnd, tempbuf);
        delete tempbuf;
        delete formatbuf;
        formatbuf = nullptr;
    }

    return (INT_PTR)TRUE;
}

INT_PTR HelperDialog::OnCommand(int id)
{
    switch (id)
    {
    case IDC_SEARCH_WEB_RADIO:
        {
            helperaction_ = SEARCHFROMWEB;
            return (INT_PTR)TRUE;
        }
        break;
    case IDC_LEARN_MORE_RADIO:
        {
            helperaction_ = LEARNMOREFROMWEB;
            return (INT_PTR)TRUE;
        }
        break;
    case IDC_SEARCH_WINDOWS_RADIO:
        {
            helperaction_ = SEARCHFROMLOCAL;
            return (INT_PTR)TRUE;
        }
        break;
    case IDB_SEARCH:
        {
            HandleSearchButton();
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;;
}

void HelperDialog::HandleSearchButton()
{
    if (::IsDlgButtonChecked(selfhwnd_, IDC_NOT_SHOW_CHECK) == BST_CHECKED)
    {
        SaveExcludedDataToReg(fileext_);
    }

    ::CoInitialize(0);
    switch (helperaction_)
    {
    case HelperDialog::SEARCHFROMLOCAL:
        {
            helperimpl_.SearchFromLocal(filepath_);
        }
        break;
    case HelperDialog::LEARNMOREFROMWEB:
    case HelperDialog::SEARCHFROMWEB:
    default:
        {
            helperimpl_.SearchFromWeb(fileext_);
        }
        break;
    }
    ::CoUninitialize();

    SendMessageW(selfhwnd_, WM_CLOSE, NULL, NULL);
}

bool HelperDialog::LoadExcludedDataFromReg(std::vector<wchar_t *> &excludeextlist)
{
    HKEY hkey;
    long ret = ::RegCreateKeyExW(HKEY_CURRENT_USER, REG_FAH_EXT_KEY, 0, nullptr, 0, KEY_READ | KEY_WOW64_64KEY, NULL, &hkey, nullptr);
    if (ret != ERROR_SUCCESS)
    {
        return false;
    }

    DWORD index = 0;
    #define DEFAULT_VALUEKEY_SIZE 256
    unsigned long real_value_key_size = DEFAULT_VALUEKEY_SIZE;
    wchar_t *value_key = nullptr;
    LSTATUS status;
    while (true)
    {
        if (value_key == nullptr)
        {
            value_key = new wchar_t[DEFAULT_VALUEKEY_SIZE];
            memset(value_key, 0, sizeof(wchar_t) * DEFAULT_VALUEKEY_SIZE);
        }

        status = ::RegEnumValueW(hkey, index, value_key, &real_value_key_size, 0, NULL, NULL, NULL);
        if(status == ERROR_MORE_DATA)
        {
            real_value_key_size++;
            delete [] value_key;
            value_key = new wchar_t[real_value_key_size];
            memset(value_key, 0, sizeof(wchar_t) * real_value_key_size);
            continue;
        }
        if (status == ERROR_SUCCESS)
        {
            excludeextlist.push_back(value_key);
            value_key = nullptr;
            index++;
            continue;
        }

        break;
    }

    return true;
}

bool HelperDialog::SaveExcludedDataToReg(const wchar_t *fileext)
{
    if (fileext == nullptr || wcslen(fileext) == 0) return false;

    HKEY hkey;
    long ret = ::RegCreateKeyExW(HKEY_CURRENT_USER, REG_FAH_EXT_KEY, 0, nullptr, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, NULL, &hkey, nullptr);
    if (ret == ERROR_SUCCESS)
    {
        ::RegSetValueExW(hkey, fileext, 0, REG_BINARY, NULL, 0);
        ::RegCloseKey(hkey);
        return true;
    }

    return false;
}

bool HelperDialog::IsExcluded(const wchar_t *fileext)
{
    if (fileext == nullptr || wcslen(fileext) == 0) return false;

    for (std::vector<wchar_t *>::iterator it = excludeextlist_.begin(); it != excludeextlist_.end(); ++it)
    {
        if (*it && _wcsicmp((*it), fileext) == 0)
            return true;
    }

    return false;
}

void HelperDialog::LoadStringFromRes(UINT resid, wchar_t **buffer)
{
    *buffer = nullptr;
    int bufsize = ::LoadStringW((HMODULE)&__ImageBase, resid, (LPWSTR)buffer, 0);
    *buffer = new wchar_t[bufsize];
    ::LoadString((HMODULE)&__ImageBase, resid, *buffer, bufsize);
}