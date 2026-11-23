#include "cg6kwin2k.h"

//
// Validation of PCI configuration access to prevent arbitrary hardware disruption
//
BOOLEAN Cg6kValidatePciAccess(
    ULONG Bus,
    ULONG Slot,
    ULONG Offset,
    ULONG Length,
    BOOLEAN IsWrite
)
{
    // Max PCI bus is 255
    if (Bus > 255) {
        return FALSE;
    }

    // Max device is 31, function is 7 (represented in slot)
    if (Slot > 255) {
        return FALSE;
    }

    if (Length == 0 || Length > 4) {
        return FALSE;
    }

    if (Offset + Length > 0x100) {
        return FALSE;
    }

    if (IsWrite) {
        ULONG start = Offset;
        ULONG end = Offset + Length;

        // Block writing to Vendor ID, Device ID (0x00 - 0x03)
        if (start < 0x04) {
            return FALSE;
        }

        // Block writing to Command & Status Register (0x04 - 0x07)
        if (start < 0x08 && end > 0x04) {
            return FALSE;
        }

        // Block writing to Class Code & Revision (0x08 - 0x0B)
        if (start < 0x0C && end > 0x08) {
            return FALSE;
        }

        // Block writing to Base Address Registers BAR0-BAR5 (0x10 - 0x27)
        if (start < 0x28 && end > 0x10) {
            return FALSE;
        }
    }

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

    // FIX: Validate PCI parameters and restrict dangerous register writes
    if (!Cg6kValidatePciAccess(pReq->Bus, pReq->Slot, pReq->Offset, Width, !IsRead)) {
        return STATUS_ACCESS_DENIED;
    }

    __try {
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
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}
