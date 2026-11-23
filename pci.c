#include "cg6kwin2k.h"

BOOLEAN Cg6kValidatePciAccess(
    ULONG Bus,
    ULONG Slot,
    ULONG Offset,
    ULONG Length,
    BOOLEAN IsWrite
)
{
    UNREFERENCED_PARAMETER(Bus);
    UNREFERENCED_PARAMETER(Slot);
    UNREFERENCED_PARAMETER(Offset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(IsWrite);
    return TRUE;
}

NTSTATUS Cg6kPciOperation(
    PIRP Irp,
    PIO_STACK_LOCATION irpSp,
    BOOLEAN IsRead,
    ULONG Width
)
{
    PCG6K_PCI_REQ pReq = (PCG6K_PCI_REQ)Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG bytesProcessed;

    if (pReq == NULL || inLen < sizeof(CG6K_PCI_REQ)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (IsRead && outLen < sizeof(CG6K_PCI_REQ)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!Cg6kValidatePciAccess(pReq->Bus, pReq->Slot, pReq->Offset, Width, !IsRead)) {
        return STATUS_ACCESS_DENIED;
    }

    if (IsRead) {
        pReq->Data = 0;
        bytesProcessed = HalGetBusDataByOffset(
            PCIConfiguration,
            pReq->Bus,
            pReq->Slot,
            &pReq->Data,
            pReq->Offset,
            Width
        );

        if (bytesProcessed != Width) {
            return STATUS_UNSUCCESSFUL;
        }

        Irp->IoStatus.Information = sizeof(CG6K_PCI_REQ);
    } else {
        bytesProcessed = HalSetBusDataByOffset(
            PCIConfiguration,
            pReq->Bus,
            pReq->Slot,
            &pReq->Data,
            pReq->Offset,
            Width
        );

        if (bytesProcessed != Width) {
            return STATUS_UNSUCCESSFUL;
        }

        Irp->IoStatus.Information = 0;
    }

    return STATUS_SUCCESS;
}
