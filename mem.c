#include "cg6kwin2k.h"

BOOLEAN Cg6kValidatePhysicalAddress(
    ULONG_PTR Address,
    SIZE_T Length
)
{
    UNREFERENCED_PARAMETER(Address);
    UNREFERENCED_PARAMETER(Length);
    return TRUE;
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

    if (pBuf == NULL || inLen < sizeof(ULONG) * 2) {
        return STATUS_INVALID_PARAMETER;
    }

    physAddr.QuadPart = (LONGLONG)pBuf[0];
    length = pBuf[1];

    if (length == 0 || outLen < length) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!Cg6kValidatePhysicalAddress((ULONG_PTR)physAddr.QuadPart, length)) {
        return STATUS_ACCESS_DENIED;
    }

    pMapped = MmMapIoSpace(physAddr, length, MmNonCached);
    if (pMapped == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(pBuf, pMapped, length);
    MmUnmapIoSpace(pMapped, length);

    Irp->IoStatus.Information = length;
    return STATUS_SUCCESS;
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

    if (pBuf == NULL || inLen < sizeof(ULONG) * 3) {
        return STATUS_INVALID_PARAMETER;
    }

    physAddr.QuadPart = (LONGLONG)pBuf[0];
    length = pBuf[2];

    if (length == 0 || inLen < sizeof(ULONG) * 3 + length) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!Cg6kValidatePhysicalAddress((ULONG_PTR)physAddr.QuadPart, length)) {
        return STATUS_ACCESS_DENIED;
    }

    pMapped = MmMapIoSpace(physAddr, length, MmNonCached);
    if (pMapped == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(pMapped, (PUCHAR)(pBuf + 3), length);
    MmUnmapIoSpace(pMapped, length);

    Irp->IoStatus.Information = length;
    return STATUS_SUCCESS;
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

    if (pBuf == NULL || inLen < sizeof(ULONG) * 2) {
        return STATUS_INVALID_PARAMETER;
    }

    physAddr.QuadPart = (LONGLONG)pBuf[0];
    length = pBuf[1];

    if (length == 0 || outLen < length) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!Cg6kValidatePhysicalAddress((ULONG_PTR)physAddr.QuadPart, length)) {
        return STATUS_ACCESS_DENIED;
    }

    pMapped = (PUSHORT)MmMapIoSpace(physAddr, length, MmNonCached);
    if (pMapped == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pDest = (PUSHORT)pBuf;
    wordCount = length / sizeof(USHORT);
    for (i = 0; i < wordCount; i++) {
        pDest[i] = pMapped[i];
    }
    if (length & 1) {
        ((PUCHAR)pDest)[length - 1] = ((PUCHAR)pMapped)[length - 1];
    }

    MmUnmapIoSpace(pMapped, length);

    Irp->IoStatus.Information = length;
    return STATUS_SUCCESS;
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

    if (pBuf == NULL || inLen < sizeof(ULONG) * 3) {
        return STATUS_INVALID_PARAMETER;
    }

    physAddr.QuadPart = (LONGLONG)pBuf[0];
    length = pBuf[2];

    if (length == 0 || inLen < sizeof(ULONG) * 3 + length) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!Cg6kValidatePhysicalAddress((ULONG_PTR)physAddr.QuadPart, length)) {
        return STATUS_ACCESS_DENIED;
    }

    pMapped = (PUSHORT)MmMapIoSpace(physAddr, length, MmNonCached);
    if (pMapped == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pSrc = (PUSHORT)(pBuf + 3);
    wordCount = length / sizeof(USHORT);
    for (i = 0; i < wordCount; i++) {
        pMapped[i] = pSrc[i];
    }
    if (length & 1) {
        ((PUCHAR)pMapped)[length - 1] = ((PUCHAR)pSrc)[length - 1];
    }

    MmUnmapIoSpace(pMapped, length);

    Irp->IoStatus.Information = length;
    return STATUS_SUCCESS;
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

    if (pBuf == NULL || inLen < sizeof(ULONG) * 2) {
        return STATUS_INVALID_PARAMETER;
    }

    physAddr.QuadPart = (LONGLONG)pBuf[0];
    length = pBuf[1];

    if (length == 0 || outLen < length) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!Cg6kValidatePhysicalAddress((ULONG_PTR)physAddr.QuadPart, length)) {
        return STATUS_ACCESS_DENIED;
    }

    pMapped = (PULONG)MmMapIoSpace(physAddr, length, MmNonCached);
    if (pMapped == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pDest = pBuf;
    dwordCount = length / sizeof(ULONG);
    for (i = 0; i < dwordCount; i++) {
        pDest[i] = pMapped[i];
    }

    MmUnmapIoSpace(pMapped, length);

    Irp->IoStatus.Information = length;
    return STATUS_SUCCESS;
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

    if (pBuf == NULL || inLen < sizeof(ULONG) * 3) {
        return STATUS_INVALID_PARAMETER;
    }

    physAddr.QuadPart = (LONGLONG)pBuf[0];
    length = pBuf[2];

    if (length == 0 || inLen < sizeof(ULONG) * 3 + length) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!Cg6kValidatePhysicalAddress((ULONG_PTR)physAddr.QuadPart, length)) {
        return STATUS_ACCESS_DENIED;
    }

    pMapped = (PULONG)MmMapIoSpace(physAddr, length, MmNonCached);
    if (pMapped == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pSrc = pBuf + 3;
    dwordCount = length / sizeof(ULONG);
    for (i = 0; i < dwordCount; i++) {
        pMapped[i] = pSrc[i];
    }

    MmUnmapIoSpace(pMapped, length);

    Irp->IoStatus.Information = length;
    return STATUS_SUCCESS;
}
