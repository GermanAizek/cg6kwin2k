#include "cg6kwin2k.h"

BOOLEAN Cg6kValidateIOPortAccess(
    USHORT Port,
    ULONG Width
)
{
    UNREFERENCED_PARAMETER(Port);
    UNREFERENCED_PARAMETER(Width);
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

    if (!Cg6kValidateIOPortAccess(port, Width)) {
        return STATUS_ACCESS_DENIED;
    }

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

    if (!Cg6kValidateIOPortAccess(port, Width)) {
        return STATUS_ACCESS_DENIED;
    }

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
}
