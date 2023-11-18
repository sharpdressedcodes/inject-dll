#include <string>
#include <Windows.h>

void msgBox(const std::wstring& msg) {
	MessageBox(nullptr, msg.c_str(), L"Injected DLL", MB_OK);
}

void onProcessAttach(HMODULE hModule) {
	wchar_t s[2048];
	wchar_t szProcessPathname[MAX_PATH];

	DisableThreadLibraryCalls(hModule);
	GetModuleFileNameW(nullptr, szProcessPathname, MAX_PATH);

	wsprintf(s, L"Attached to process (%d)\r\n%s", GetCurrentProcessId(), szProcessPathname);

	msgBox(s);
}

void onProcessDetach() {
	wchar_t s[2048];
	wchar_t szProcessPathname[MAX_PATH];

	GetModuleFileNameW(nullptr, szProcessPathname, MAX_PATH);

	wsprintf(s, L"Detached from process (%d)\r\n%s", GetCurrentProcessId(), szProcessPathname);

	msgBox(s);
}

void onThreadAttach() {
	wchar_t s[2048];
	wchar_t szProcessPathname[MAX_PATH] = { 0 };
	HANDLE hThread = GetCurrentThread();
	const DWORD tid = GetCurrentThreadId();
	const DWORD pid = GetProcessIdOfThread(hThread);
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, false, pid);

	if (!hProcess) {
		hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid);
	}

	if (hProcess) {
		DWORD size = MAX_PATH;
		QueryFullProcessImageName(hProcess, 0, szProcessPathname, &size);
		CloseHandle(hProcess);
	}
	
	wsprintf(s, L"Attached thread (%d) to process (%d)\r\n%s", tid, pid, szProcessPathname);

	msgBox(s);
}

void onThreadDetach() {
	wchar_t s[2048];
	wchar_t szProcessPathname[MAX_PATH] = { 0 };
	HANDLE hThread = GetCurrentThread();
	const DWORD tid = GetCurrentThreadId();
	const DWORD pid = GetProcessIdOfThread(hThread);
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, false, pid);

	if (!hProcess) {
		hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid);
	}

	if (hProcess) {
		DWORD size = MAX_PATH;
		QueryFullProcessImageName(hProcess, 0, szProcessPathname, &size);
		CloseHandle(hProcess);
	}

	wsprintf(s, L"Detached thread (%d) from process (%d)\r\n%s", tid, pid, szProcessPathname);

	msgBox(s);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReasonForCall, LPVOID lpReserved) {
    switch (ulReasonForCall) {
	    case DLL_PROCESS_ATTACH:
			onProcessAttach(hModule);
			break;

		case DLL_PROCESS_DETACH:
			onProcessDetach();
			break;
    	
	    case DLL_THREAD_ATTACH:
			onThreadAttach();
			break;
    	
	    case DLL_THREAD_DETACH:
			onThreadDetach();
	        break;
    }
	
    return TRUE;
}
