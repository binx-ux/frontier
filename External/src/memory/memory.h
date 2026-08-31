#pragma once
#include <windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <memory>

#ifdef FRONTIER_KERNEL
#include "driver_link.h"
#endif

#ifndef FRONTIER_KERNEL
extern "C" intptr_t
Luck_ReadVirtualMemory
(
	HANDLE ProcessHandle,
	PVOID BaseAddress,
	PVOID Buffer,
	ULONG NumberOfBytesToRead,
	PULONG NumberOfBytesRead
);

extern "C" intptr_t
Luck_WriteVirtualMemory
(
	HANDLE Processhandle,
	PVOID BaseAddress,
	PVOID Buffer,
	ULONG NumberOfBytesToWrite,
	PULONG NumberOfBytesWritten
);
#endif

class memory_t final
{
public:
	memory_t() = default;
	~memory_t() = default;

	std::uint32_t find_process_id(const std::string& process_name);
	std::uint64_t find_module_address(const std::string& module_name);

	bool attach_to_process(const std::string& process_name);

	std::string read_string(std::uint64_t address);
	bool IsConnected();

	template <typename T>
	T read(std::uint64_t address);

	template <typename T>
	void write(std::uint64_t address, T value);

	std::uint32_t get_process_id();
	std::uint64_t get_module_address();
	std::uint64_t get_module_size();
	HANDLE get_process_handle();
private:
	bool read_raw(std::uint64_t address, void* buffer, std::size_t size);
	bool write_raw(std::uint64_t address, const void* buffer, std::size_t size);

	std::uint32_t process_id = 0;
	std::uint64_t base_address = 0;
	std::uint64_t module_size = 0;
	HANDLE process_handle = nullptr;
};

template <typename T>
T memory_t::read(uint64_t address)
{
	T buffer{};
	read_raw(address, &buffer, sizeof(T));
	return buffer;
}

template <typename T>
void memory_t::write(uint64_t address, T value)
{
	write_raw(address, &value, sizeof(T));
}

extern std::unique_ptr<memory_t> memory;