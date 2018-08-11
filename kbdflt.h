
#ifndef _KBDFLT_H_
#define	_KBDFLT_H_

/////////////////////////////////////////////////////////////////////
//	H E A D E R S.
/////////////////////////////////////////////////////////////////////

#include "ntddk.h"


/////////////////////////////////////////////////////////////////////
//	E X T E R N  V A R I A B L E S.
/////////////////////////////////////////////////////////////////////

extern ULONG g_ulPendingIrps;


/////////////////////////////////////////////////////////////////////
//	S T R U C T U R E S.
/////////////////////////////////////////////////////////////////////

//
//	Structure for keyboard input data.
//
typedef struct tagKEYBOARD_INPUT_DATA
{
	USHORT	UnitId;
	USHORT	MakeCode;
	USHORT	Flags;
	USHORT	Reserved;
	ULONG	ExtraInformation;

}	KEYBOARD_INPUT_DATA, *PKEYBOARD_INPUT_DATA;

//
//	Structure for device extension.
//
typedef struct tagDEVICE_EXTENSION
{
	PDEVICE_OBJECT	pKeyboardDevice;

}	DEVICE_EXTENSION, *PDEVICE_EXTENSION;



/////////////////////////////////////////////////////////////////////
//	F U N C T I O N  D E C L A R A T I O N S.
/////////////////////////////////////////////////////////////////////

NTSTATUS
HookKeyboard(
	PDRIVER_OBJECT pDriverObject
	);


NTSTATUS
DispathUnload(
	PDRIVER_OBJECT pDriverObject
	);


NTSTATUS
DispathRead(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
	);


NTSTATUS
OnReadCompletetion(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp,
	PVOID Context
	);


NTSTATUS
DispatchPassdown(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
	);



#endif	//	_KBDFLT_H_

