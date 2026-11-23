#include "cg6kwin2k.h"

//
// Whitelist and validation for x86 hardware I/O ports
//
BOOLEAN Cg6kValidateIOPortAccess(
    USHORT Port,
    ULONG Width,
    BOOLEAN IsWrite
)
{
    // Check width
    if (Width != 1 && Width != 2 && Width != 4) {
        return FALSE;
    }

    // Check for overflow
    if ((ULONG)Port + Width > 0x10000) {
        return FALSE;
    }

    // Block critical legacy controllers that can cause system crashes, DMA attacks, or exploit SMM
    if ((Port <= 0x001F) ||                     // Master DMA 8237
        (Port >= 0x0020 && Port <= 0x0021) ||   // Master PIC 8259
        (Port >= 0x0040 && Port <= 0x0043) ||   // PIT 8254
        (Port == 0x0060 || Port == 0x0064) ||   // Keyboard Controller 8042
        (Port >= 0x0070 && Port <= 0x0071) ||   // RTC / CMOS
        (Port >= 0x0080 && Port <= 0x008F && Port != 0x0080 && Port != 0x0081) || // DMA page registers (allow 0x80/0x81 POST)
        (Port >= 0x00A0 && Port <= 0x00A1) ||   // Slave PIC 8259
        (Port == 0x00B2 || Port == 0x00B3) ||   // APM SMI control
        (Port >= 0x00C0 && Port <= 0x00DF)) {   // Slave DMA 8237
        return FALSE;
    }

    if (IsWrite) {
        // Safe diagnostic, PCI configuration, and board I/O ports:
        // - POST diagnostic ports: 0x80, 0x81, 0xEB
        // - PCI Configuration Address / Data: 0xCF8 - 0xCFC
        // - Board I/O ranges (0x0300 - 0x03FF)
        if (Port == 0x0080 || Port == 0x0081 || Port == 0x00EB ||
            (Port >= 0x0CF8 && Port <= 0x0CFC) ||
            (Port >= 0x0300 && Port <= 0x03FF)) {
            return TRUE;
        }
        return FALSE;
    }

    return TRUE;
}

NTSTATUS Cg6kReadPort(
    PIRP Irp,
    PIO_STACK_LOCATION irpSp,
    ULONG Width
)
{
    PCG6K_PORT_REQ pReq = (PCG6K_PORT_REQ)Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    USHORT port;

    if (pReq == NULL || inLen < sizeof(ULONG) || outLen < sizeof(ULONG)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    port = (USHORT)(pReq->Port & 0xFFFF);

    // FIX: Validate I/O port read access to prevent hardware controller hijacking
    if (!Cg6kValidateIOPortAccess(port, Width, FALSE)) {
        return STATUS_ACCESS_DENIED;
    }

    __try {
        switch (Width) {
        case 1:
            pReq->Data = (ULONG)READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)port);
            break;
        case 2:
            pReq->Data = (ULONG)READ_PORT_USHORT((PUSHORT)(ULONG_PTR)port);
            break;
        case 4:
            pReq->Data = READ_PORT_ULONG((PULONG)(ULONG_PTR)port);
            break;
        default:
            return STATUS_INVALID_PARAMETER;
        }

        Irp->IoStatus.Information = sizeof(ULONG);
        return STATUS_SUCCESS;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}

NTSTATUS Cg6kWritePort(
    PIRP Irp,
    PIO_STACK_LOCATION irpSp,
    ULONG Width
)
{
    PCG6K_PORT_REQ pReq = (PCG6K_PORT_REQ)Irp->AssociatedIrp.SystemBuffer;
    ULONG inLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    USHORT port;

    if (pReq == NULL || inLen < sizeof(CG6K_PORT_REQ)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    port = (USHORT)(pReq->Port & 0xFFFF);

    // FIX: Validate I/O port write access to block dangerous controllers and APM SMI
    if (!Cg6kValidateIOPortAccess(port, Width, TRUE)) {
        return STATUS_ACCESS_DENIED;
    }

    __try {
        switch (Width) {
        case 1:
            WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)port, (UCHAR)(pReq->Data & 0xFF));
            break;
        case 2:
            WRITE_PORT_USHORT((PUSHORT)(ULONG_PTR)port, (USHORT)(pReq->Data & 0xFFFF));
            break;
        case 4:
            WRITE_PORT_ULONG((PULONG)(ULONG_PTR)port, pReq->Data);
            break;
        default:
            return STATUS_INVALID_PARAMETER;
        }

        Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}
