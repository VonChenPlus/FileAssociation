#pragma once

class HelperImpl
{
public:
    HelperImpl(void);
    ~HelperImpl(void);

    bool OpenHelperDialog(const wchar_t *filepath);
    bool SearchFromLocal(const wchar_t *filepath);
    bool SearchFromWeb(const wchar_t *fileext);

private:
    void FormatString(const wchar_t *src, wchar_t **dest);
};

