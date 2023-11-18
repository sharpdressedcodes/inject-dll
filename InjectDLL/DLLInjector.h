#pragma once

#include <string>
#include <Windows.h>
#include <TlHelp32.h>

class DLLInjector {
public:
	static HMODULE getModuleFromProcess(const DWORD& pid, const std::wstring& dllPath) {
		HMODULE hModule = nullptr;
		MODULEENTRY32 me = { sizeof(MODULEENTRY32) };
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);

		if (hSnapshot == INVALID_HANDLE_VALUE) {
			return hModule;
		}

		for (BOOL b = Module32First(hSnapshot, &me); b; b = Module32Next(hSnapshot, &me)) {
			if (_wcsicmp(me.szExePath, dllPath.c_str()) == 0 || _wcsicmp(me.szModule, dllPath.c_str()) == 0) {
				hModule = me.hModule;
				break;
			}
		}

		CloseHandle(hSnapshot);
		
		return hModule;
	}

	static bool isInjected(const DWORD& pid, const std::wstring& dllPath) {
		return getModuleFromProcess(pid, dllPath) != nullptr;
	}

	static std::wstring eject(const DWORD& pid, const std::wstring& dllPath) {
		DWORD threadId = 0;
		DWORD threadExitCode = 0;
		HMODULE kernel32Handle = GetModuleHandle(L"Kernel32");

		if (!kernel32Handle) {
			return L"Error getting Kernel32 handle.";
		}

		HMODULE dllHandle = getModuleFromProcess(pid, dllPath);

		if (!dllHandle) {
			return L"Error finding dll handle, has it been injected?";
		}

		const DWORD dwDesiredAccess =
			PROCESS_QUERY_INFORMATION | // Required by Alpha
			PROCESS_CREATE_THREAD |		// For CreateRemoteThread
			PROCESS_VM_OPERATION;		// For VirtualAllocEx and VirtualFreeEx

		HANDLE processHandle = OpenProcess(dwDesiredAccess, false, pid);

		if (!processHandle) {
			return L"Error opening process.";
		}

		PTHREAD_START_ROUTINE address = (PTHREAD_START_ROUTINE)GetProcAddress(kernel32Handle, "FreeLibrary");

		if (!address) {
			CloseHandle(processHandle);
			return L"Error getting proc address for FreeLibrary.";
		}

		HANDLE threadHandle = CreateRemoteThread(processHandle, nullptr, 0, address, dllHandle, 0, &threadId);

		if (!threadHandle) {
			CloseHandle(processHandle);
			
			return L"Error creating remote thread.";
		}

		WaitForSingleObject(threadHandle, INFINITE);
		
		GetExitCodeThread(threadHandle, &threadExitCode);

		CloseHandle(threadHandle);
		CloseHandle(processHandle);

		return std::wstring();
	}

	static std::wstring inject(const DWORD& pid, const std::wstring& dllPath) {
		DWORD threadId = 0;
		DWORD threadExitCode = 0;
		const size_t bufferSize = (dllPath.size() + 1) * sizeof(wchar_t);
		HMODULE kernel32Handle = GetModuleHandle(L"Kernel32");

		if (!kernel32Handle) {
			return L"Error getting Kernel32 handle.";
		}

		const DWORD dwDesiredAccess =
			PROCESS_QUERY_INFORMATION | // Required by Alpha
			PROCESS_CREATE_THREAD |		// For CreateRemoteThread
			PROCESS_VM_OPERATION |		// For VirtualAllocEx and VirtualFreeEx
			PROCESS_VM_WRITE;			// For WriteProcessMemory

		HANDLE processHandle = OpenProcess(dwDesiredAccess, false, pid);

		// If OpenProcess returns NULL, the application is not running under a security context that allows it to open a handle to the
		// target process. Some processes - such as WinLogon, SvcHost, and Csrss - run under the local system account, which
		// the logged - on user cannot alter. You might be able to open a handle to these processes if you are granted and enable the
		// debug security privilege. The ProcessInfo sample in Chapter 4, "Processes," demonstrates how to do this.

		if (!processHandle) {
			return L"Error opening process.";
		}

		PVOID buffer = VirtualAllocEx(processHandle, NULL, bufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		if (!buffer) {
			CloseHandle(processHandle);
			
			return L"Error allocating virtual memory.";
		}

		if (!WriteProcessMemory(processHandle, buffer, dllPath.c_str(), bufferSize, NULL)) {
			VirtualFreeEx(processHandle, buffer, 0, MEM_RELEASE);
			CloseHandle(processHandle);
			
			return L"Error writing to virtual memory.";
		}

		PTHREAD_START_ROUTINE address = (PTHREAD_START_ROUTINE)GetProcAddress(kernel32Handle, "LoadLibraryW");

		if (!address) {
			VirtualFreeEx(processHandle, buffer, 0, MEM_RELEASE);
			CloseHandle(processHandle);
			
			return L"Error getting proc address for LoadLibraryW.";
		}

		HANDLE threadHandle = CreateRemoteThread(processHandle, NULL, 0, address, buffer, 0, &threadId);

		if (!threadHandle) {
			VirtualFreeEx(processHandle, buffer, 0, MEM_RELEASE);
			CloseHandle(processHandle);
			
			return L"Error creating remote thread.";
		}

		WaitForSingleObject(threadHandle, INFINITE);

		GetExitCodeThread(threadHandle, &threadExitCode);

		VirtualFreeEx(processHandle, buffer, 0, MEM_RELEASE);
		CloseHandle(threadHandle);
		CloseHandle(processHandle);

		return std::wstring();
	}
};
