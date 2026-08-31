#include <ntddk.h>
#include <ntstrsafe.h>

#define DEVICE_NAME L"\\Device\\FrontierDrv"
#define SYMLINK_NAME L"\\DosDevices\\Global\\FrontierDrv"
#define SYMLINK_NAME_LEGACY L"\\DosDevices\\FrontierDrv"

#define IOCTL_FRONTIER_READ_MEMORY \
    CTL_CODE(0x8000, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_FRONTIER_WRITE_MEMORY \
    CTL_CODE(0x8000, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FRONTIER_RW_MAX 4096u

#pragma pack(push, 1)
typedef struct _FRONTIER_MEMORY_PACKET {
    ULONG ProcessId;
    ULONGLONG Address;
    ULONG Size;
    UCHAR Data[FRONTIER_RW_MAX];
} FRONTIER_MEMORY_PACKET, *PFRONTIER_MEMORY_PACKET;
#pragma pack(pop)

NTKERNELAPI NTSTATUS NTAPI MmCopyVirtualMemory(
    PEPROCESS SourceProcess,
    PVOID SourceAddress,
    PEPROCESS TargetProcess,
    PVOID TargetAddress,
    SIZE_T BufferSize,
    KPROCESSOR_MODE PreviousMode,
    PSIZE_T ReturnSize);

NTKERNELAPI NTSTATUS NTAPI PsLookupProcessByProcessId(
    HANDLE ProcessId,
    PEPROCESS* Process);

static PDEVICE_OBJECT gDeviceObject = NULL;

static NTSTATUS CopyProcessMemory(
    ULONG processId,
    ULONGLONG address,
    PVOID buffer,
    SIZE_T size,
    BOOLEAN write)
{
    PEPROCESS process = NULL;
    NTSTATUS status = PsLookupProcessByProcessId((HANDLE)(ULONG_PTR)processId, &process);
    if (!NT_SUCCESS(status))
        return status;

    SIZE_T transferred = 0;
    if (write) {
        status = MmCopyVirtualMemory(
            PsGetCurrentProcess(),
            buffer,
            process,
            (PVOID)(ULONG_PTR)address,
            size,
            KernelMode,
            &transferred);
    } else {
        status = MmCopyVirtualMemory(
            process,
            (PVOID)(ULONG_PTR)address,
            PsGetCurrentProcess(),
            buffer,
            size,
            KernelMode,
            &transferred);
    }

    ObDereferenceObject(process);
    return status;
}

static NTSTATUS HandleIoctl(PFRONTIER_MEMORY_PACKET packet, ULONG code)
{
    if (!packet || packet->Size == 0 || packet->Size > FRONTIER_RW_MAX)
        return STATUS_INVALID_PARAMETER;

    if (code == IOCTL_FRONTIER_READ_MEMORY) {
        return CopyProcessMemory(
            packet->ProcessId,
            packet->Address,
            packet->Data,
            packet->Size,
            FALSE);
    }

    if (code == IOCTL_FRONTIER_WRITE_MEMORY) {
        return CopyProcessMemory(
            packet->ProcessId,
            packet->Address,
            (PVOID)packet->Data,
            packet->Size,
            TRUE);
    }

    return STATUS_INVALID_DEVICE_REQUEST;
}

static NTSTATUS DispatchCreate(PDEVICE_OBJECT deviceObject, PIRP irp)
{
    UNREFERENCED_PARAMETER(deviceObject);
    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS DispatchClose(PDEVICE_OBJECT deviceObject, PIRP irp)
{
    UNREFERENCED_PARAMETER(deviceObject);
    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS DispatchDeviceControl(PDEVICE_OBJECT deviceObject, PIRP irp)
{
    UNREFERENCED_PARAMETER(deviceObject);

    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(irp);
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    PVOID buffer = irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = stack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLen = stack->Parameters.DeviceIoControl.OutputBufferLength;

    if (!buffer || inLen < sizeof(FRONTIER_MEMORY_PACKET) || outLen < sizeof(FRONTIER_MEMORY_PACKET)) {
        irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        irp->IoStatus.Information = 0;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_BUFFER_TOO_SMALL;
    }

    NTSTATUS status = HandleIoctl((PFRONTIER_MEMORY_PACKET)buffer, code);
    irp->IoStatus.Status = status;
    irp->IoStatus.Information = NT_SUCCESS(status) ? sizeof(FRONTIER_MEMORY_PACKET) : 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return status;
}

static VOID DriverUnload(PDRIVER_OBJECT driverObject)
{
    UNREFERENCED_PARAMETER(driverObject);
    UNICODE_STRING symlink;
    RtlInitUnicodeString(&symlink, SYMLINK_NAME);
    IoDeleteSymbolicLink(&symlink);
    RtlInitUnicodeString(&symlink, SYMLINK_NAME_LEGACY);
    IoDeleteSymbolicLink(&symlink);
    if (gDeviceObject)
        IoDeleteDevice(gDeviceObject);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath)
{
    UNREFERENCED_PARAMETER(registryPath);

    UNICODE_STRING deviceName;
    RtlInitUnicodeString(&deviceName, DEVICE_NAME);

    NTSTATUS status = IoCreateDevice(
        driverObject,
        0,
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &gDeviceObject);

    if (!NT_SUCCESS(status))
        return status;

    UNICODE_STRING symlink;
    RtlInitUnicodeString(&symlink, SYMLINK_NAME);
    status = IoCreateSymbolicLink(&symlink, &deviceName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(gDeviceObject);
        gDeviceObject = NULL;
        return status;
    }

    RtlInitUnicodeString(&symlink, SYMLINK_NAME_LEGACY);
    (void)IoCreateSymbolicLink(&symlink, &deviceName);

    driverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
    driverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
    driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;
    driverObject->DriverUnload = DriverUnload;
    gDeviceObject->Flags |= DO_BUFFERED_IO;
    gDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}
