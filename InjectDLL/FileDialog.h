#pragma once

#include <filesystem>
#include <string>
#include <Windows.h>
#define STRICT_TYPED_ITEMIDS
#include <objbase.h>
#include <ShObjIdl.h>
#include <Shlwapi.h>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Shlwapi.lib")

//#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#ifdef UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

class FileDialogEventHandler : public IFileDialogEvents, public IFileDialogControlEvents {
public:
	FileDialogEventHandler() : m_cRef(1) {}

	// IUnknown methods
	IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)  {
		static const QITAB qit[] = {
			QITABENT(FileDialogEventHandler, IFileDialogEvents),
			QITABENT(FileDialogEventHandler, IFileDialogControlEvents),
			{ 0 },
#pragma warning(suppress:4838)
		};

		return QISearch(this, qit, riid, ppv);
	}

	IFACEMETHODIMP_(ULONG) AddRef() {
		return InterlockedIncrement(&m_cRef);
	}

	IFACEMETHODIMP_(ULONG) Release() {
		const long cRef = InterlockedDecrement(&m_cRef);
		
		if (!cRef)
			delete this;
		
		return cRef;
	}

	// IFileDialogEvents methods
	IFACEMETHODIMP OnFileOk(IFileDialog* fileDialog) {
		return S_OK;
		
		IShellItem* psiResult;
		PWSTR pszFilePath = NULL;
		IFileOpenDialog* pfod;
		HRESULT hr = fileDialog->QueryInterface(IID_PPV_ARGS(&pfod));

		hr = pfod->GetResult(&psiResult);
		hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

		// if (pszFilePath && wcslen(pszFilePath) > 0) {
		// 	//result = std::wstring(pszFilePath);
		// 	hr = S_OK;
		// }

		hr = pszFilePath && wcslen(pszFilePath) > 0 ? S_OK : S_FALSE;

		psiResult->Release();
		CoTaskMemFree(pszFilePath);
		
		return hr;//S_FALSE; //S_OK;
	}
	IFACEMETHODIMP OnFolderChange(IFileDialog*) { return S_OK; }
	IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*) { return S_OK; }
	IFACEMETHODIMP OnSelectionChange(IFileDialog*) { return S_OK; }
	IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) { return S_OK; }
	IFACEMETHODIMP OnTypeChange(IFileDialog* pfd) { return S_OK; }
	IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) { return S_OK; }

	// IFileDialogControlEvents methods
	IFACEMETHODIMP OnItemSelected(IFileDialogCustomize*, DWORD, DWORD) { return S_OK; }
	IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize*, DWORD) { return S_OK; }
	IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize*, DWORD, BOOL) { return S_OK; }
	IFACEMETHODIMP OnControlActivating(IFileDialogCustomize*, DWORD) { return S_OK; }
	
	static HRESULT createInstance(REFIID riid, void** ppv) {
		*ppv = NULL;
		auto handler = new FileDialogEventHandler();

		if (!handler) {
			return E_OUTOFMEMORY;
		}

		const HRESULT hr = handler->QueryInterface(riid, ppv);
		
		handler->Release();
		
		return hr;
	}
protected:
	long m_cRef = 0;

	~FileDialogEventHandler() {}
};

/**
 * Notes:
 * This class needs C++17 or newer to compile.
 *
 * Usage:
 * #include "FileDialog.h"
 * 
 * std::shared_ptr<FileDialog> fileDialog;
 *
 * HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
 *
 * const COMDLG_FILTERSPEC c_rgSaveTypes[] = {
 *	 { L"Dynamic Link Library (*.dll)", L"*.dll" },
 *	 { L"All Documents (*.*)", L"*.*" }
 * };
 * 
 * fileDialog.reset(new FileDialog());
 * 
 * fileDialog->getInterface()->SetFileTypes({
 *   { L"Dynamic Link Library (*.dll)", L"*.dll" },
 *   { L"All Documents (*.*)", L"*.*" }
 * });
 * fileDialog->getInterface()->SetFileTypeIndex(0);
 * fileDialog->getInterface()->SetDefaultExtension(L"doc;docx");
 * std::wstring result = fileDialog->show(initialPath, hwnd);
 * 
 * if (temp.empty()) {
 *     fileDialog.reset();
 *     CoUninitialize();
 *
 *     return 1;
 * }
 *
 */

class FileDialog {
public:
	FileDialog() {
		HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
	}
	
	~FileDialog() {
		pfd->Release();
	}

	[[nodiscard]] IFileDialog* getInterface() const {
		return pfd;
	}
	
	std::wstring show(const std::wstring& initialPath = std::wstring(), HWND hwnd = NULL) {
		IFileDialogEvents* pfde = NULL;
		DWORD dwCookie = 0;
		DWORD dwFlags;
		std::wstring result;
		bool b = false;
		IShellItem* pCurFolder = NULL;
		// HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));

		HRESULT hr = pfd->GetOptions(&dwFlags);
		hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
		//hr = pfd->SetFileTypes(ARRAYSIZE(c_rgSaveTypes), c_rgSaveTypes);
		//hr = pfd->SetFileTypeIndex(INDEX_WORDDOC);
		//hr = pfd->SetDefaultExtension(L"dll"); // (L"doc;docx");

		hr = FileDialogEventHandler::createInstance(IID_PPV_ARGS(&pfde));
		hr = pfd->Advise(pfde, &dwCookie);

		if (std::filesystem::is_regular_file(initialPath)) {
			hr = pfd->SetFileName(initialPath.c_str());
		}

		std::wstring p = std::filesystem::is_directory(initialPath) ? initialPath : getFilePath(initialPath);

		if (p.empty()) {
			p = getExecutablePath();
		}

		hr = SHCreateItemFromParsingName(p.c_str(), NULL, IID_PPV_ARGS(&pCurFolder));

		if (SUCCEEDED(hr)) {
			pfd->SetFolder(pCurFolder);
			pCurFolder->Release();
		}
		
		hr = pfd->Show(hwnd);

		if (FAILED(hr)) {
			// User clicked cancel.
			pfd->Unadvise(dwCookie);
			return L"";
		}

		IShellItem* psiResult;
		PWSTR pszFilePath = NULL;

		hr = pfd->GetResult(&psiResult);
		hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

		if (pszFilePath) {
			result = std::wstring(pszFilePath);
		}

		psiResult->Release();
		CoTaskMemFree(pszFilePath);

		pfd->Unadvise(dwCookie);
		pfde->Release();

		return result;
	}

protected:
	IFileDialog* pfd = NULL;
	
	std::wstring getFilePath(const std::wstring& path) {
		const size_t pos = path.find_last_of(L"\\/");

		return std::wstring::npos == pos ? path : path.substr(0, pos);
	}

	std::wstring getExecutablePath() {
		wchar_t buffer[MAX_PATH] = { 0 };

		GetModuleFileName(NULL, buffer, MAX_PATH);

		return getFilePath(buffer);
	}
};