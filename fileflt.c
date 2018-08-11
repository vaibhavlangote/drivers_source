#include "fileflt.h"


//
//	Filter handle.
//
PFLT_FILTER g_pFilter;

//
//  Assign text sections for each routine.
//
#ifdef ALLOC_PRAGMA
    #pragma alloc_text(INIT, DriverEntry)
    #pragma alloc_text(PAGE, FileFltPreCreate)
    #pragma alloc_text(PAGE, FilterQueryTeardown)
    #pragma alloc_text(PAGE, FilterUnload)
    #pragma alloc_text(PAGE, FilterInstanceSetup)
#endif

typedef struct tagFILEFLT_STREAM_HANDLE_CONTEXT
{
	BOOLEAN bIsAttached;

} FILEFLT_STREAM_HANDLE_CONTEXT, *P_FILEFLT_STREAM_HANDLE_CONTEXT;


const FLT_CONTEXT_REGISTRATION contextRegistration[] = {
	//{
	//	FLT_STREAMHANDLE_CONTEXT,
	//		0,
	//		NULL,
	//		sizeof(FILEFLT_STREAM_HANDLE_CONTEXT),
	//		'csff'
	//},

	{
		FLT_CONTEXT_END
	}
};


const FLT_OPERATION_REGISTRATION Callbacks[] = 
{
	{
		IRP_MJ_CREATE,
		0,
		FileFltPreCreate,
		FileFltPostCreate
	},

	{
		IRP_MJ_OPERATION_END
	}
};


const FLT_REGISTRATION FilterRegistration = {
	sizeof(FLT_REGISTRATION),
	FLT_REGISTRATION_VERSION,
	NULL,
	contextRegistration,
	Callbacks,
	FilterUnload,
	FilterInstanceSetup,
	FilterQueryTeardown,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
	};


FLT_PREOP_CALLBACK_STATUS
FileFltPreCreate(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__deref_out_opt PVOID *CompletionContext
	)
{
    PAGED_CODE();

	DbgPrint("FileFltPreCreate==>>");
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


FLT_POSTOP_CALLBACK_STATUS
FileFltPostCreate(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in_opt PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags
	)
{
	DbgPrint("FileFltPostCreate==>>");
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


NTSTATUS
FilterUnload (
    __in FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for the Filter driver.  This unregisters the
    Filter with the filter manager and frees any allocated global data
    structures.

Arguments:

    None.

Return Value:

    Returns the final status of the deallocation routines.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

	DbgPrint("FilterUnload==>>");

    //
    //  Unregister the filter
    //

    FltUnregisterFilter(g_pFilter);

    return STATUS_SUCCESS;
}


NTSTATUS
FilterInstanceSetup(
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_SETUP_FLAGS Flags,
    __in DEVICE_TYPE VolumeDeviceType,
    __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called by the filter manager when a new instance is created.
    We specified in the registry that we only want for manual attachments,
    so that is all we should receive here.

Arguments:

    FltObjects - Describes the instance and volume which we are being asked to
        setup.

    Flags - Flags describing the type of attachment this is.

    VolumeDeviceType - The DEVICE_TYPE for the volume to which this instance
        will attach.

    VolumeFileSystemType - The file system formatted on this volume.

Return Value:

  FLT_NOTIFY_STATUS_ATTACH              - we wish to attach to the volume
  FLT_NOTIFY_STATUS_DO_NOT_ATTACH       - no, thank you

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

	DbgPrint("FilterInstanceSetup==>>");

    //
    //  Don't attach to network volumes.
    //

    //if (VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM) {

    //   return STATUS_FLT_DO_NOT_ATTACH;
    //}

    return STATUS_SUCCESS;
}

NTSTATUS
FilterQueryTeardown(
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is the instance detach routine for the filter. This
    routine is called by filter manager when a user initiates a manual instance
    detach. This is a 'query' routine: if the filter does not want to support
    manual detach, it can return a failure status

Arguments:

    FltObjects - Describes the instance and volume for which we are receiving
        this query teardown request.

    Flags - Unused

Return Value:

    STATUS_SUCCESS - we allow instance detach to happen

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

	DbgPrint("FilterQueryTeardown==>>.\n");
	DbgPrint("<<==FilterQueryTeardown.\n");

    return STATUS_SUCCESS;
}

NTSTATUS
DriverEntry(
	PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath
	)
{
	NTSTATUS Status = STATUS_SUCCESS;

	DbgPrint("DriverEntry==>>.\n");

	Status = FltRegisterFilter(DriverObject, &FilterRegistration, &g_pFilter);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("FltRegisterFilter (0x%X)...FAILED.\n", Status);
		return Status;
	}
	else
	{
		DbgPrint("FltRegisterFilter...SUCCESS.\n");
	}

	Status = FltStartFiltering(g_pFilter);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("FltStartFiltering (0x%X)...FAILED.\n", Status);
		return Status;
	}
	else
	{
		DbgPrint("FltStartFiltering...SUCCESS.\n");
	}

	DbgPrint("<<==DriverEntry.\n");

	return STATUS_SUCCESS;
}
