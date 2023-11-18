#include <clocale>
#include <filesystem>
#include <future>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <Windows.h>
#include <codecvt>
#include "FileDialog.h"
#include "DLLInjector.h"

std::shared_ptr<FileDialog> fileDialog;

bool tryParse(std::string& input, int& output) {
	try {
		output = std::stoi(input);
	} catch (std::invalid_argument&) {
		return false;
	}
	return true;
}

int getProcessIdFromUser() {
	std::string input;
	int x;

	std::cout << "Enter a Process ID: ";
	getline(std::cin, input);

	while (!tryParse(input, x)) {
		std::cout << "Bad entry. Enter a Process ID: ";
		getline(std::cin, input);
	}
	
	return x;
}

std::wstring getDllPathFromUser(const std::wstring& path) {
	const COMDLG_FILTERSPEC c_rgSaveTypes[] = {
		{ L"Dynamic Link Library (*.dll)", L"*.dll" },
		{ L"All Documents (*.*)", L"*.*" }
	};

	fileDialog.reset(new FileDialog());
	
	fileDialog->getInterface()->SetFileTypes(ARRAYSIZE(c_rgSaveTypes), c_rgSaveTypes);
	fileDialog->getInterface()->SetFileTypeIndex(0);
	fileDialog->getInterface()->SetDefaultExtension(L"dll");

	return fileDialog->show(path);
}

int wmain(int argc, wchar_t* argv[]) {
	DWORD pid = 0;
	std::wstring dllPath;
	using namespace std::literals;
    std::wfstream configFile;
    std::wstring temp;
    const std::locale defaultLocale("");
    const std::wstring fileName = L"InjectDLLConfig.txt";

    std::setlocale(LC_ALL, "");

    configFile.imbue(defaultLocale);
    configFile.open(fileName, std::ios::in | std::ios::out);

	if (!configFile) {
        std::wcout << L"Can't open config file " << fileName << std::endl;
        return 1;
	}

    configFile >> temp;
    configFile.clear();
	
    resize_file(std::filesystem::path(fileName), 0);
    configFile.seekp(0);

	if (!temp.empty()) {
		dllPath = temp;
	}
	
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	
	switch (argc) {	
	case 2:
		pid = getProcessIdFromUser();
		dllPath = argv[1];
		break;
		
	case 3:
		pid = std::stoi(argv[1]);
		dllPath = argv[2];
		break;

	default:
		pid = getProcessIdFromUser();
        temp = getDllPathFromUser(dllPath);

		if (temp.empty()) {
			fileDialog.reset();
            CoUninitialize();
			
            return 1;
		}
		
		dllPath = temp;
	}
	
	if (DLLInjector::isInjected(pid, dllPath)) {
		std::cout << "This process has already been injected." << std::endl;
		fileDialog.reset();
        CoUninitialize();

        configFile << dllPath;
		return 1;
	}

	std::cout << "Ensure to close any message boxes that open." << std::endl;

	std::wstring injectorResult = DLLInjector::inject(pid, dllPath);

	if (!injectorResult.empty()) {
		std::cout << "Error injecting into process with pid " << pid << std::endl;
		std::wcout << injectorResult << std::endl;
		fileDialog.reset();
        CoUninitialize();

        configFile << dllPath;
		return 1;
	}

	std::cout << "Successfully injected process with pid " << pid << std::endl
			  << "Press any key then ENTER to end." << std::endl;
	
	const auto f = std::async(std::launch::async, [] {
		auto s = L""s;
		
		if (std::wcin >> s) {
			return s;
		}

		return s;
	});
	
	while (f.wait_for(2s) != std::future_status::ready) {
		// Do nothing
	}

	std::cout << "Ensure to close any message boxes that open." << std::endl;
	
	injectorResult = DLLInjector::eject(pid, dllPath);

	if (!injectorResult.empty()) {
		std::cout << "Error unloading injected DLL from target process with pid " << pid << std::endl;
		std::wcout << injectorResult << std::endl;
	} else {
		std::cout << "Successfully unloaded DLL from process with pid " << pid << std::endl;
	}

	fileDialog.reset();

	CoUninitialize();

    configFile << dllPath;

	return 0;
}