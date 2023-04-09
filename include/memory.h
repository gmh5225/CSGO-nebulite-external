#pragma once
#include <cstdint>
#include <string_view>
#include <Windows.h>
#include <TlHelp32.h>
#include <codecvt>
#include <locale> 

class Memory {
private:
	std::uintptr_t processId = 0;
	void* processHandle = nullptr;
public:
	Memory(const std::string_view processName) noexcept {
		HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPMODULE, 0);

		if (handle) {
			PROCESSENTRY32W process{};
			process.dwSize = sizeof(process);

			if (Process32FirstW(handle, &process)) {
				do
				{
					if (_wcsicmp(process.szExeFile, std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(processName.data()).c_str()) == 0) {
						processId = process.th32ProcessID;
						processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
						break;
					}
				} while (Process32NextW(handle, &process));
			}
		}
	}

	const std::uintptr_t GetModuleAddress(const std::string_view moduleName) const noexcept {
		MODULEENTRY32 entry{};
		entry.dwSize = sizeof(MODULEENTRY32);

		const auto processSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
		std::uintptr_t result = 0;

		while (Module32Next(processSnapshot, &entry)) {
			if (_stricmp(entry.szModule, moduleName.data()) == 0) {
				result = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
				break;
			}
		}

		if (processSnapshot)
			CloseHandle(processSnapshot);

		return result;
	}

	// Read process memory
	template <typename T>
	constexpr const T Read(const std::uintptr_t& address) const noexcept {
		T value = { };
		ReadProcessMemory(processHandle, reinterpret_cast<const void*>(address), &value, sizeof(T), NULL);

		return value;
	}

	// Write process memory
	template <typename T>
	constexpr void Write(const std::uintptr_t& address, const T& value) const noexcept{
		WriteProcessMemory(processHandle, reinterpret_cast<void*>(address), &value, sizeof(T), NULL);
	}
};