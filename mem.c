#include "cg6kwin2k.h"

BOOLEAN Cg6kValidatePhysicalAddress(
    ULONG_PTR Address,
    SIZE_T Length
)
{
    // Check for overflow
    if (Address + Length < Address) {
        return FALSE;
    }

    // Limit maximum transfer size (64KB)
    if (Length == 0 || Length > 0x10000) {
        return FALSE;
    }

    // Block lower physical memory containing IVT, BDA, and kernel zero-page
    if (Address < 0x000A0000) {
        return FALSE;
    }

    // Allow BIOS / Option ROM / Video RAM area: 0x000A0000 - 0x00100000
    if (Address >= 0x000A0000 && (Address + Length) <= 0x00100000) {
        return TRUE;
    }

    // Allow hardware MMIO address space (PCI BARs / hardware registers)
    if (Address >= 0xC0000000) {
        return TRUE;
    }

    // Block arbitrary system RAM access
    return FALSE;
}

NTSTATUS Cg6kReadPhysicalMemory(
    PIRP Irp,
    PIO_STACK_LOCATION irpSp
)
{
    PULONG pBuf = (PULONG)Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PHYSICAL_ADDRESS physAddr;
    ULONG length;
    PVOID pMapped = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    if (pBuf == NULL || inLen < sizeof(ULONG) * 2) {
        return STATUS_INVALID_PARAMETER;
    }

    physAddr.QuadPart = (LONGLONG)pBuf[0];
    length = pBuf[1];

    if (length == 0 || outLen < length) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    // FIX: Restrict arbitrary physical memory read to safe hardware MMIO and BIOS ranges
    if (!Cg6kValidatePhysicalAddress((ULONG_PTR)physAddr.QuadPart, length)) {
        return STATUS_ACCESS_DENIED;
    }

    pMapped = MmMapIoSpace(physAddr, length, MmNonCached);
    if (pMapped == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    __try {
        __try {
            RtlCopyMemory(pBuf, pMapped, length);
            Irp->IoStatus.Information = length;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
        }
    } __finally {
        MmUnmapIoSpace(pMapped, length);
    }

    return status;
}

NTSTATUS Cg6kWritePhysicalMemory(
    PIRP Irp,
    PIO_STACK_LOCATION irpSp
)
{
    PULONG pBuf = (PULONG)Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    PHYSICAL_ADDRESS physAddr;
    ULONG length;
    PVOID pMapped = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    if (pBuf == NULL || inLen < sizeof(ULONG) * 3) {
        return STATUS_INVALID_PARAMETER;
    }

    physAddr.QuadPart = (LONGLONG)pBuf[0];
    length = pBuf[2];

    if (length == 0 || inLen < sizeof(ULONG) * 3 + length) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    // FIX: Restrict arbitrary physical memory write to safe hardware MMIO ranges
    if (!Cg6kValidatePhysicalAddress((ULONG_PTR)physAddr.QuadPart, length)) {
        return STATUS_ACCESS_DENIED;
    }

    pMapped = MmMapIoSpace(physAddr, length, MmNonCached);
    if (pMapped == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    __try {
        __try {
            RtlCopyMemory(pMapped, (PUCHAR)(pBuf + 3), length);
            Irp->IoStatus.Information = length;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
        }
    } __finally {
        MmUnmapIoSpace(pMapped, length);
    }

    return status;
}

NTSTATUS Cg6kReadPhysicalWord(
    PIRP Irp,
    PIO_STACK_LOCATION irpSp
)
{
    PULONG pBuf = (PULONG)Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PHYSICAL_ADDRESS physAddr;
    ULONG length;
    PUSHORT pMapped = NULL;
    PUSHORT pDest;
    ULONG wordCount;
    ULONG i;
    NTSTATUS status = STATUS_SUCCESS;

    if (pBuf == NULL || inLen < sizeof(ULONG) * 2) {
        return STATUS_INVALID_PARAMETER;
    }

    physAddr.QuadPart = (LONGLONG)pBuf[0];
    length = pBuf[1];

    if (length == 0 || outLen < length) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    // FIX: Enforce 2-byte alignment for word read access
    if ((physAddr.QuadPart & 1) != 0) {
        return STATUS_DATATYPE_MISALIGNMENT;
    }

    // FIX: Restrict arbitrary physical memory word read to safe hardware MMIO ranges
    if (!Cg6kValidatePhysicalAddress((ULONG_PTR)physAddr.QuadPart, length)) {
        return STATUS_ACCESS_DENIED;
    }

    pMapped = (PUSHORT)MmMapIoSpace(physAddr, length, MmNonCached);
    if (pMapped == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    __try {
        __try {
            pDest = (PUSHORT)pBuf;
            wordCount = length / sizeof(USHORT);
            for (i = 0; i < wordCount; i++) {
                pDest[i] = pMapped[i];
            }
            if (length & 1) {
                ((PUCHAR)pDest)[length - 1] = ((PUCHAR)pMapped)[length - 1];
            }
            Irp->IoStatus.Information = length;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
        }
    } __finally {
        MmUnmapIoSpace(pMapped, length);
    }

    return status;
}

NTSTATUS Cg6kWritePhysicalWord(
    PIRP Irp,
    PIO_STACK_LOCATION irpSp
)
{
    PULONG pBuf = (PULONG)Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    PHYSICAL_ADDRESS physAddr;
    ULONG length;
    PUSHORT pMapped = NULL;
    PUSHORT pSrc;
    ULONG wordCount;
    ULONG i;
    NTSTATUS status = STATUS_SUCCESS;

    if (pBuf == NULL || inLen < sizeof(ULONG) * 3) {
        return STATUS_INVALID_PARAMETER;
    }

    physAddr.QuadPart = (LONGLONG)pBuf[0];
    length = pBuf[2];

    if (length == 0 || inLen < sizeof(ULONG) * 3 + length) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    // FIX: Enforce 2-byte alignment for word write access
    if ((physAddr.QuadPart & 1) != 0) {
        return STATUS_DATATYPE_MISALIGNMENT;
    }

    // FIX: Restrict arbitrary physical memory word write to safe hardware MMIO ranges
    if (!Cg6kValidatePhysicalAddress((ULONG_PTR)physAddr.QuadPart, length)) {
        return STATUS_ACCESS_DENIED;
    }

    pMapped = (PUSHORT)MmMapIoSpace(physAddr, length, MmNonCached);
    if (pMapped == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    __try {
        __try {
            pSrc = (PUSHORT)(pBuf + 3);
            wordCount = length / sizeof(USHORT);
            for (i = 0; i < wordCount; i++) {
                pMapped[i] = pSrc[i];
            }
            if (length & 1) {
                ((PUCHAR)pMapped)[length - 1] = ((PUCHAR)pSrc)[length - 1];
            }
            Irp->IoStatus.Information = length;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
        }
    } __finally {
        MmUnmapIoSpace(pMapped, length);
    }

    return status;
}

NTSTATUS Cg6kReadPhysicalDword(
    PIRP Irp,
    PIO_STACK_LOCATION irpSp
)
{
    PULONG pBuf = (PULONG)Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PHYSICAL_ADDRESS physAddr;
    ULONG length;
    PULONG pMapped = NULL;
    PULONG pDest;
    ULONG dwordCount;
    ULONG i;
    NTSTATUS status = STATUS_SUCCESS;

    if (pBuf == NULL || inLen < sizeof(ULONG) * 2) {
        return STATUS_INVALID_PARAMETER;
    }

    physAddr.QuadPart = (LONGLONG)pBuf[0];
    length = pBuf[1];

    if (length == 0 || outLen < length) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    // FIX: Enforce 4-byte alignment for dword read access
    if ((physAddr.QuadPart & 3) != 0 || (length & 3) != 0) {
        return STATUS_DATATYPE_MISALIGNMENT;
    }

    // FIX: Restrict arbitrary physical memory dword read to safe hardware MMIO ranges
    if (!Cg6kValidatePhysicalAddress((ULONG_PTR)physAddr.QuadPart, length)) {
        return STATUS_ACCESS_DENIED;
    }

    pMapped = (PULONG)MmMapIoSpace(physAddr, length, MmNonCached);
    if (pMapped == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    __try {
        __try {
            pDest = pBuf;
            dwordCount = length / sizeof(ULONG);
            for (i = 0; i < dwordCount; i++) {
                pDest[i] = pMapped[i];
            }
            Irp->IoStatus.Information = length;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
        }
    } __finally {
        MmUnmapIoSpace(pMapped, length);
    }

    return status;
}

NTSTATUS Cg6kWritePhysicalDword(
    PIRP Irp,
    PIO_STACK_LOCATION irpSp
)
{
    PULONG pBuf = (PULONG)Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    PHYSICAL_ADDRESS physAddr;
    ULONG length;
    PULONG pMapped = NULL;
    PULONG pSrc;
    ULONG dwordCount;
    ULONG i;
    NTSTATUS status = STATUS_SUCCESS;

    if (pBuf == NULL || inLen < sizeof(ULONG) * 3) {
        return STATUS_INVALID_PARAMETER;
    }

    physAddr.QuadPart = (LONGLONG)pBuf[0];
    length = pBuf[2];

    if (length == 0 || inLen < sizeof(ULONG) * 3 + length) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    // FIX: Enforce 4-byte alignment for dword write access
    if ((physAddr.QuadPart & 3) != 0 || (length & 3) != 0) {
        return STATUS_DATATYPE_MISALIGNMENT;
    }

    // FIX: Restrict arbitrary physical memory dword write to safe hardware MMIO ranges
    if (!Cg6kValidatePhysicalAddress((ULONG_PTR)physAddr.QuadPart, length)) {
        return STATUS_ACCESS_DENIED;
    }

    pMapped = (PULONG)MmMapIoSpace(physAddr, length, MmNonCached);
    if (pMapped == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    __try {
        __try {
            pSrc = pBuf + 3;
            dwordCount = length / sizeof(ULONG);
            for (i = 0; i < dwordCount; i++) {
                pMapped[i] = pSrc[i];
            }
            Irp->IoStatus.Information = length;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
        }
    } __finally {
        MmUnmapIoSpace(pMapped, length);
    }

    return status;
}
