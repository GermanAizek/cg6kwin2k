#pragma once

#include <ntddk.h>
#include <wdm.h>
#include <wdmsec.h>
#include <ntstrsafe.h>

#define CG6K_DEVICE_NAME            L"\\Device\\CG6K"
#define CG6K_DOS_DEVICE_NAME        L"\\DosDevices\\CG6K"
#define CG6K_DEVICE_TYPE            0x00000022  // FILE_DEVICE_UNKNOWN

#define TAG_CG6K_POOL               'K6GC'      // 0x4B364743 ('CG6K')

//
// Board IOCTL definitions
//
#define IOCTL_CG6K_BOARD_STATUS         0x002A2800
#define IOCTL_CG6K_MSG_RECEIVE          0x80012004
#define IOCTL_CG6K_MSG_BUFFER           0x8001200A
#define IOCTL_CG6K_MSG_FLUSH            0x8001200C
#define IOCTL_CG6K_MSG_SEND             0x80012011
#define IOCTL_CG6K_GET_MAP_INFO         0x80012014
#define IOCTL_CG6K_SET_SLOT             0x80012018
#define IOCTL_CG6K_START_BOARD          0x8001201C
#define IOCTL_CG6K_STOP_BOARD           0x80012020
#define IOCTL_CG6K_GET_VERSION          0x80012024
#define IOCTL_CG6K_EEPROM_READ          0x80012028
#define IOCTL_CG6K_EEPROM_WRITE         0x8001202C
#define IOCTL_CG6K_REGISTER_BOARD       0x80012030
#define IOCTL_CG6K_DEREGISTER_BOARD     0x80012034
#define IOCTL_CG6K_ENUM_BOARDS          0x80012038
#define IOCTL_CG6K_GET_STATS            0x80012040
#define IOCTL_CG6K_CLEAR_STATS          0x80012044
#define IOCTL_CG6K_GET_QUEUE_ID         0x80012048
#define IOCTL_CG6K_SET_QUEUE_MODE       0x8001204C
#define IOCTL_CG6K_CONFIG_CHANNEL       0x80012050
#define IOCTL_CG6K_CONFIG_CHANNEL_ALT   0x80012054
#define IOCTL_CG6K_CONTROL_TRACE        0x8001205C

//
// Hardware Access IOCTL definitions (FUN_0001484c)
//
// PCI Configuration
#define IOCTL_CG6K_PCI_WRITE_BYTE       0x80012EE0
#define IOCTL_CG6K_PCI_WRITE_WORD       0x80012F00
#define IOCTL_CG6K_PCI_WRITE_DWORD      0x80012F04
#define IOCTL_CG6K_PCI_READ_WORD        0x80012F08
#define IOCTL_CG6K_PCI_READ_DWORD       0x80012F0C
#define IOCTL_CG6K_PCI_READ_BYTE        0x80012F20

// Physical Memory
#define IOCTL_CG6K_WRITE_PHYSICAL_WORD  0x80012EE4
#define IOCTL_CG6K_WRITE_PHYSICAL_WORD2 0x80012EF0
#define IOCTL_CG6K_WRITE_PHYSICAL_DWORD 0x80012EF4
#define IOCTL_CG6K_READ_PHYSICAL_WORD   0x80012EE8
#define IOCTL_CG6K_READ_PHYSICAL_WORD2  0x80012F10
#define IOCTL_CG6K_READ_PHYSICAL_DWORD  0x80012F14
#define IOCTL_CG6K_READ_PHYSICAL_MEM    0x80012F18
#define IOCTL_CG6K_WRITE_PHYSICAL_MEM   0x80012F1C

// Port I/O
#define IOCTL_CG6K_PORT_WRITE_DWORD     0x80012EF8
#define IOCTL_CG6K_PORT_WRITE_WORD      0x80012EFC
#define IOCTL_CG6K_PORT_WRITE_BYTE      0x80012F28
#define IOCTL_CG6K_PORT_READ_BYTE       0x80012F2C
#define IOCTL_CG6K_PORT_READ_WORD       0x80012F30
#define IOCTL_CG6K_PORT_READ_DWORD      0x80012F34

// Board & Diagnostic IOCTLs
#define IOCTL_CG6K_HW_NOP               0x80012F38
#define IOCTL_CG6K_GET_HW_VERSION       0x80012F3C
#define IOCTL_CG6K_DIAG_TRANSFER        0x80012F40
#define IOCTL_CG6K_BOARD_INFO_READ      0x80012F44
#define IOCTL_CG6K_BOARD_INFO_WRITE     0x80012F48

#pragma pack(push, 1)

typedef struct _CG6K_PORT_REQ {
    ULONG Port;      // Port address (low 16 bits)
    ULONG Data;      // Data value to write, or received data
} CG6K_PORT_REQ, *PCG6K_PORT_REQ;

typedef struct _CG6K_PCI_REQ {
    ULONG Bus;       // Bus / Board index
    ULONG Slot;      // Slot / Device / Function
    ULONG Offset;    // Configuration register offset
    ULONG Data;      // Data to write or buffer
} CG6K_PCI_REQ, *PCG6K_PCI_REQ;

typedef struct _CG6K_VERSION_INFO {
    ULONG Size;
    ULONG MajorVersion;
    ULONG MinorVersion;
} CG6K_VERSION_INFO, *PCG6K_VERSION_INFO;

typedef struct _CG6K_DEVICE_EXTENSION {
    PDEVICE_OBJECT DeviceObject;
    PDRIVER_OBJECT DriverObject;
    KTIMER Timer;
    KDPC Dpc;
    BOOLEAN TimerActive;
    ULONG BoardState;
    ULONG BoardNumber;
    ULONG QueueMode;
} CG6K_DEVICE_EXTENSION, *PCG6K_DEVICE_EXTENSION;

#pragma pack(pop)

//
// Global state
//
extern PDEVICE_OBJECT g_DeviceObject;
extern PDRIVER_OBJECT g_DriverObject;

//
// Function prototypes
//

// main.c
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD Cg6kUnload;
DRIVER_DISPATCH Cg6kCreateClose;
DRIVER_DISPATCH Cg6kDispatchIoControl;
KDEFERRED_ROUTINE Cg6kTimerDpc;

// mem.c
NTSTATUS Cg6kReadPhysicalMemory(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS Cg6kWritePhysicalMemory(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS Cg6kReadPhysicalWord(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS Cg6kWritePhysicalWord(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS Cg6kReadPhysicalDword(PIRP Irp, PIO_STACK_LOCATION irpSp);
NTSTATUS Cg6kWritePhysicalDword(PIRP Irp, PIO_STACK_LOCATION irpSp);
BOOLEAN Cg6kValidatePhysicalAddress(ULONG_PTR Address, SIZE_T Length);

// io.c
NTSTATUS Cg6kReadPort(PIRP Irp, PIO_STACK_LOCATION irpSp, ULONG Width);
NTSTATUS Cg6kWritePort(PIRP Irp, PIO_STACK_LOCATION irpSp, ULONG Width);
BOOLEAN Cg6kValidateIOPortAccess(USHORT Port, ULONG Width, BOOLEAN IsWrite);

// pci.c
NTSTATUS Cg6kPciOperation(PIRP Irp, PIO_STACK_LOCATION irpSp, BOOLEAN IsRead, ULONG Width);
BOOLEAN Cg6kValidatePciAccess(ULONG Bus, ULONG Slot, ULONG Offset, ULONG Length, BOOLEAN IsWrite);
