#include "cg6kwin2k.h"

// Device Class GUID for cg6kwin2k (LOLDrivers: fda3ff67-12dc-488c-a4c2-603f5bf420bd)
static const GUID GUID_DEVCLASS_CG6K = {
    0xFDA3FF67, 0x12DC, 0x488C, { 0xA4, 0xC2, 0x60, 0x3F, 0x5B, 0xF4, 0x20, 0xBD }
};

PDEVICE_OBJECT g_DeviceObject = NULL;
PDRIVER_OBJECT g_DriverObject = NULL;

VOID Cg6kTimerDpc(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);
}

NTSTATUS Cg6kCreateClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

VOID Cg6kUnload(
    PDRIVER_OBJECT DriverObject
)
{
    UNICODE_STRING dosDeviceName;
    PCG6K_DEVICE_EXTENSION devExt = NULL;

    if (g_DeviceObject != NULL) {
        devExt = (PCG6K_DEVICE_EXTENSION)g_DeviceObject->DeviceExtension;
        if (devExt != NULL && devExt->TimerActive) {
            KeCancelTimer(&devExt->Timer);
            devExt->TimerActive = FALSE;
        }

        RtlInitUnicodeString(&dosDeviceName, CG6K_DOS_DEVICE_NAME);
        IoDeleteSymbolicLink(&dosDeviceName);

        // FIX: Unconditionally delete device object to prevent resource leak on unload
        IoDeleteDevice(g_DeviceObject);
        g_DeviceObject = NULL;
    }

    UNREFERENCED_PARAMETER(DriverObject);
}

NTSTATUS Cg6kDispatchIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION irpSp;
    ULONG ioctlCode;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG_PTR information = 0;
    PVOID buffer;
    ULONG inLength;
    ULONG outLength;

    UNREFERENCED_PARAMETER(DeviceObject);

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    ioctlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;
    buffer = Irp->AssociatedIrp.SystemBuffer;
    inLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    outLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    UNREFERENCED_PARAMETER(inLength);

    switch (ioctlCode) {
    case IOCTL_CG6K_BOARD_STATUS:
        if (buffer != NULL && outLength >= sizeof(ULONG)) {
            *(PULONG)buffer = 0x00000005; // Ready & initialized
            information = sizeof(ULONG);
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case IOCTL_CG6K_GET_VERSION:
        if (buffer != NULL && outLength >= sizeof(CG6K_VERSION_INFO)) {
            PCG6K_VERSION_INFO pVer = (PCG6K_VERSION_INFO)buffer;
            pVer->Size = sizeof(CG6K_VERSION_INFO);
            pVer->MajorVersion = 2;
            pVer->MinorVersion = 0;
            information = sizeof(CG6K_VERSION_INFO);
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case IOCTL_CG6K_GET_HW_VERSION:
        if (buffer != NULL && outLength >= sizeof(ULONG)) {
            *(PULONG)buffer = 0x00010000;
            information = sizeof(ULONG);
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    case IOCTL_CG6K_HW_NOP:
        status = STATUS_SUCCESS;
        information = 0;
        break;

    // Physical Memory IOCTLs
    case IOCTL_CG6K_READ_PHYSICAL_MEM:
        status = Cg6kReadPhysicalMemory(Irp, irpSp);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_CG6K_WRITE_PHYSICAL_MEM:
        status = Cg6kWritePhysicalMemory(Irp, irpSp);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_CG6K_READ_PHYSICAL_WORD:
    case IOCTL_CG6K_READ_PHYSICAL_WORD2:
        status = Cg6kReadPhysicalWord(Irp, irpSp);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_CG6K_WRITE_PHYSICAL_WORD:
    case IOCTL_CG6K_WRITE_PHYSICAL_WORD2:
        status = Cg6kWritePhysicalWord(Irp, irpSp);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_CG6K_READ_PHYSICAL_DWORD:
        status = Cg6kReadPhysicalDword(Irp, irpSp);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_CG6K_WRITE_PHYSICAL_DWORD:
        status = Cg6kWritePhysicalDword(Irp, irpSp);
        information = Irp->IoStatus.Information;
        break;

    // Port I/O IOCTLs
    case IOCTL_CG6K_PORT_READ_BYTE:
        status = Cg6kReadPort(Irp, irpSp, 1);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_CG6K_PORT_READ_WORD:
        status = Cg6kReadPort(Irp, irpSp, 2);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_CG6K_PORT_READ_DWORD:
        status = Cg6kReadPort(Irp, irpSp, 4);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_CG6K_PORT_WRITE_BYTE:
        status = Cg6kWritePort(Irp, irpSp, 1);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_CG6K_PORT_WRITE_WORD:
        status = Cg6kWritePort(Irp, irpSp, 2);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_CG6K_PORT_WRITE_DWORD:
        status = Cg6kWritePort(Irp, irpSp, 4);
        information = Irp->IoStatus.Information;
        break;

    // PCI Configuration IOCTLs
    case IOCTL_CG6K_PCI_READ_BYTE:
        status = Cg6kPciOperation(Irp, irpSp, TRUE, 1);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_CG6K_PCI_READ_WORD:
        status = Cg6kPciOperation(Irp, irpSp, TRUE, 2);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_CG6K_PCI_READ_DWORD:
        status = Cg6kPciOperation(Irp, irpSp, TRUE, 4);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_CG6K_PCI_WRITE_BYTE:
        status = Cg6kPciOperation(Irp, irpSp, FALSE, 1);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_CG6K_PCI_WRITE_WORD:
        status = Cg6kPciOperation(Irp, irpSp, FALSE, 2);
        information = Irp->IoStatus.Information;
        break;

    case IOCTL_CG6K_PCI_WRITE_DWORD:
        status = Cg6kPciOperation(Irp, irpSp, FALSE, 4);
        information = Irp->IoStatus.Information;
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        information = 0;
        break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = information;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    UNICODE_STRING dosDeviceName;
    UNICODE_STRING sddl;
    PCG6K_DEVICE_EXTENSION devExt;
    LARGE_INTEGER dueTime;

    UNREFERENCED_PARAMETER(RegistryPath);

    g_DriverObject = DriverObject;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = Cg6kCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = Cg6kCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Cg6kDispatchIoControl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = Cg6kDispatchIoControl;
    DriverObject->DriverUnload = Cg6kUnload;

    RtlInitUnicodeString(&deviceName, CG6K_DEVICE_NAME);
    RtlInitUnicodeString(&dosDeviceName, CG6K_DOS_DEVICE_NAME);

    // FIX: Secure device creation to mitigate unprivileged access (LOLDrivers: cg6kwin2k.sys, CVE-2024-29216)
    // SDDL: D:P(A;;GA;;;SY)(A;;GA;;;BA)
    RtlInitUnicodeString(&sddl, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)");

    status = IoCreateDeviceSecure(
        DriverObject,
        sizeof(CG6K_DEVICE_EXTENSION),
        &deviceName,
        CG6K_DEVICE_TYPE,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &sddl,
        &GUID_DEVCLASS_CG6K,
        &g_DeviceObject
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = IoCreateSymbolicLink(&dosDeviceName, &deviceName);
    if (!NT_SUCCESS(status)) {
        // FIX: Clean up device object on symbolic link creation failure
        IoDeleteDevice(g_DeviceObject);
        g_DeviceObject = NULL;
        return status;
    }

    devExt = (PCG6K_DEVICE_EXTENSION)g_DeviceObject->DeviceExtension;
    RtlZeroMemory(devExt, sizeof(CG6K_DEVICE_EXTENSION));
    devExt->DeviceObject = g_DeviceObject;
    devExt->DriverObject = DriverObject;

    KeInitializeTimer(&devExt->Timer);
    KeInitializeDpc(&devExt->Dpc, Cg6kTimerDpc, devExt);
    dueTime.QuadPart = -10000000LL; // 1 second
    KeSetTimerEx(&devExt->Timer, dueTime, 1000, &devExt->Dpc);
    devExt->TimerActive = TRUE;

    return STATUS_SUCCESS;
}
