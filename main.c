/////////////////////////////////////////////////////////////////////
//	H E A D E R S.
/////////////////////////////////////////////////////////////////////

#include "kbdflt.h"


/////////////////////////////////////////////////////////////////////
//	G L O B A L S.
/////////////////////////////////////////////////////////////////////

ULONG g_ulPendingIrps = 0;


/////////////////////////////////////////////////////////////////////
//	F U N C T I O N  D E F I N A T I O N S.
/////////////////////////////////////////////////////////////////////

//
//	This function use to hook keyboard class driver.
//
NTSTATUS
HookKeyboard(
	PDRIVER_OBJECT pDriverObject
	)
{
	NTSTATUS ntStatus;
	UNICODE_STRING uKeyboardDeviceName;
	PDEVICE_EXTENSION pDeviceExtension;
	PDEVICE_OBJECT pKeyboardDeviceObject;	// The filter device object.

	DbgPrint("==>HookKeyboard\n");

	ntStatus = IoCreateDevice(
							pDriverObject,
							sizeof(DEVICE_EXTENSION),
							NULL, // no name.
							FILE_DEVICE_KEYBOARD,
							0,
							TRUE,
							&pKeyboardDeviceObject
							);
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("IoCreateDevice...FAILED.\n");

		return ntStatus;
	}

	pKeyboardDeviceObject->Flags = pKeyboardDeviceObject->Flags | (DO_BUFFERED_IO | DO_POWER_PAGABLE);
	pKeyboardDeviceObject->Flags = pKeyboardDeviceObject->Flags & ~DO_DEVICE_INITIALIZING;

	//
	//	Initialize device extension memory with zero.
	//	And get pointer to the device extension.
	//
	RtlZeroMemory(pKeyboardDeviceObject->DeviceExtension, sizeof(DEVICE_EXTENSION));
	pDeviceExtension = (PDEVICE_EXTENSION)pKeyboardDeviceObject->DeviceExtension;

	RtlInitUnicodeString(&uKeyboardDeviceName, L"\\Device\\KeyboardClass0");

	IoAttachDevice(
					pKeyboardDeviceObject,
					&uKeyboardDeviceName,
					&pDeviceExtension->pKeyboardDevice
					);

	//RtlFreeUnicodeString(&uKeyboardDeviceName);

	DbgPrint("HookKeyboard<==\n");

	return STATUS_SUCCESS;
}


//
//	Dispatch unload function.
//
NTSTATUS
DispathUnload(
	PDRIVER_OBJECT pDriverObject
	)
{
	KTIMER kTimerObj;
	LARGE_INTEGER timeOut;
	PDEVICE_EXTENSION pKeyboardExtension;

	DbgPrint("==>DispatchUnload\n");

	pKeyboardExtension = (PDEVICE_EXTENSION)pDriverObject->DeviceObject->DeviceExtension;
	IoDetachDevice(pKeyboardExtension->pKeyboardDevice);

	timeOut.QuadPart = 1000000;	//	.1 s
	KeInitializeTimer(&kTimerObj);

	DbgPrint("DisatchUnlod(Pending IRP : %u)\n", g_ulPendingIrps);

	while (g_ulPendingIrps > 0)
	{

		KeSetTimer(&kTimerObj, timeOut, NULL);

		DbgPrint("DisatchUnlod in loop(Pending IRP : %u)\n", g_ulPendingIrps);

		KeWaitForSingleObject(
							&kTimerObj,
							Executive,
							KernelMode,
							FALSE,
							NULL
							);
	}

	DbgPrint("DisatchUnlod(Pending IRP : %u)\n", g_ulPendingIrps);

	IoDeleteDevice(pDriverObject->DeviceObject);

	DbgPrint("DispatchUnload<==\n");

	return STATUS_SUCCESS;
}


//
//	Dispatch routine for read request.
//
NTSTATUS
DispathRead(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
	)
{
	NTSTATUS ntStaus;

	DbgPrint("==>DispathRead\n");

	//
	//	Copy parameters down to the next level of stack.
	//
	IoCopyCurrentIrpStackLocationToNext(pIrp);

	IoSetCompletionRoutine(pIrp,
							OnReadCompletetion,
							pDeviceObject,
							TRUE,
							TRUE,
							TRUE
							);

	g_ulPendingIrps++;	//	Track pending IRPS.

	DbgPrint("PendingIrps: %u\n", g_ulPendingIrps);

	DbgPrint("DispathRead<==\n");

	return IoCallDriver(((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->pKeyboardDevice, pIrp);
}



//
//	Io completion call back for read operation.
//
NTSTATUS
OnReadCompletetion(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp,
	PVOID Context
	)
{
	int iIndex;
	int iNumKeys;
	PKEYBOARD_INPUT_DATA pKeyboardInputData;

	DbgPrint("==>OnReadCompletetion\n");

	if (pIrp->IoStatus.Status == STATUS_SUCCESS)
	{
		pKeyboardInputData = (PKEYBOARD_INPUT_DATA)pIrp->AssociatedIrp.SystemBuffer;
		iNumKeys = pIrp->IoStatus.Information / sizeof(KEYBOARD_INPUT_DATA);
		for (iIndex = 0; iIndex < iNumKeys; iIndex++)
		{
			DbgPrint("ScanCode:%x\n", pKeyboardInputData[iIndex].MakeCode);
		}
	}

	if (pIrp->PendingReturned)
	{
		IoMarkIrpPending(pIrp);
	}

	g_ulPendingIrps--;

	DbgPrint("OnReadCompletetion<==\n");

	return pIrp->IoStatus.Status;;
}


//
//	Dispatch routine for all other major functions.
//
NTSTATUS
DispatchPassdown(
	PDEVICE_OBJECT pDeviceObject,
	PIRP pIrp
	)
{
	return STATUS_SUCCESS;
}


NTSTATUS
DriverEntry(
	PDRIVER_OBJECT DriverObj,
	PUNICODE_STRING RegPath
	)
{
	int i;
	NTSTATUS ntStatus = STATUS_SUCCESS;

	DbgPrint("==> DriverEntry\n");

	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObj->MajorFunction[i] = DispatchPassdown;
	}

	DriverObj->MajorFunction[IRP_MJ_READ] = DispathRead;
	DriverObj->DriverUnload = DispathUnload;

	HookKeyboard(DriverObj);

	DbgPrint("DriverEntry <==\n");

	return ntStatus;
}
