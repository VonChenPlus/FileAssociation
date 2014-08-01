#pragma once

class FileAssociationHelper
{
private:
    static UINT shellhookmsg_;
    static HANDLE stopevent_;

public:
    static bool RunStartCmd();
    static bool RunStopCmd();

private:
    static HRESULT CreateProcessTrack();
    static void ProcessStartCallback(DWORD processid, const wchar_t *processname);
};