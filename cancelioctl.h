
#ifndef __CANCELIOCTL_H__
#define __CANCELIOCTL_H__

#define	CANCELQ_DEVICE_NAME_W	L"\\Device\\CancelQ"
#define	CANCELQ_DOS_DEVICE_NAME_W	L"\\DosDevices\\CancelQ"


#define	CANCEL_DEVICE_ID	53360

#define	IOCTL_READ_DATA\
	CTL_CODE(CANCEL_DEVICE_ID, 2021, METHOD_BUFFERED, FILE_ANY_ACCESS)


#endif //	__CANCELIOCTL_H__
