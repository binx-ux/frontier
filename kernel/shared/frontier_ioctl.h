#pragma once

// Shared between kernel driver and usermode client (FRONTIER_KERNEL builds).

#define FRONTIER_DEVICE_PATH L"\\\\.\\FrontierDrv"
#define FRONTIER_DRIVER_SERVICE_NAME L"FrontierDrv"
#define FRONTIER_DRIVER_FILE_NAME L"FrontierDrv.sys"

#define FRONTIER_IOCTL_BASE 0x8000

#define IOCTL_FRONTIER_READ_MEMORY \
    CTL_CODE(FRONTIER_IOCTL_BASE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_FRONTIER_WRITE_MEMORY \
    CTL_CODE(FRONTIER_IOCTL_BASE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FRONTIER_RW_MAX 4096u

#pragma pack(push, 1)
typedef struct _FRONTIER_MEMORY_PACKET {
    unsigned long ProcessId;
    unsigned long long Address;
    unsigned long Size;
    unsigned char Data[FRONTIER_RW_MAX];
} FRONTIER_MEMORY_PACKET, *PFRONTIER_MEMORY_PACKET;
#pragma pack(pop)
