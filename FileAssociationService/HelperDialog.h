#pragma once
#include <Windows.h>
#include <vector>
#include "HelperImpl.h"

class HelperDialog
{
private:
    HWND parenthwnd_;
    HWND selfhwnd_;
    wchar_t filepath_[MAX_PATH + 1];
    wchar_t *filename_;
    wchar_t *fileext_;
    std::vector<wchar_t *> excludeextlist_;
    typedef enum HelperAction
    {
        SEARCHFROMWEB,
        LEARNMOREFROMWEB,
        SEARCHFROMLOCAL,
    };
    HelperAction helperaction_;
    HelperImpl helperimpl_;

public:
    HelperDialog(void);
    ~HelperDialog(void);

    bool DoMoadl(HWND parenthwnd, const wchar_t *filepath);
private:
    static HelperDialog *GetDialogData(HWND hwnd, int index);
    static void SetDialogData(HWND hwnd, int index, HelperDialog *data);

    static INT_PTR CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    INT_PTR OnInitDialog(void);
    INT_PTR OnCommand(int id);
    void HandleSearchButton();

    bool LoadExcludedDataFromReg(std::vector<wchar_t *> &excludeextlist);
    bool SaveExcludedDataToReg(const wchar_t *fileext);
    bool IsExcluded(const wchar_t *fileext);

    void LoadStringFromRes(UINT resid, wchar_t **buffer);
};

