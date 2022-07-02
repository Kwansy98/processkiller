#include <ntifs.h>
#include "EzProcess.h"
#include "EzOsVersion.h"
#include "EzThread.h"
#include "EzString.h"

#define _DEVICE_NAME L"\\device\\processkiller"
#define _SYM_NAME L"\\??\\processkiller"

#define KILL_PROCESS 1

#define CODE_CTR_INDEX 0x800
#define MY_CTL_CODE CTL_CODE(FILE_DEVICE_UNKNOWN,CODE_CTR_INDEX,METHOD_NEITHER,FILE_ANY_ACCESS)

VOID KillThreadCallback(IN PETHREAD Thread, IN PVOID Context);
VOID KillProcessCallback(IN PEPROCESS Process, IN PVOID Context);

// ͨ��ͨ�Ű�
typedef struct  _CMD
{
	ULONG64 code;
	ULONG64  in;
	ULONG64  inLen;
	ULONG64  out;
	ULONG64  outLen;
}CMD, * PCMD;

// �Զ���ͨ������

typedef struct  _PROCESSINFO
{
	ULONG64 Pids[1024];
	int Cnt;
	BOOLEAN KillSameName;
	BOOLEAN NoReopen;
}PROCESSINFO, * PPROCESSINFO;

// �������еĽ���һ������ɱ��
#define MAX_BLACKLIST_LEN 400
UNICODE_STRING ProcessBlackList[MAX_BLACKLIST_LEN] = { 0 };
int ProcessBlackListCnt = 0;
KSPIN_LOCK ProcessBlackListLock;

VOID ClearProcessBlackList()
{
	KIRQL irql = { 0 };
	KeAcquireSpinLock(&ProcessBlackListLock, &irql);
	for (int i = 0; i < ProcessBlackListCnt; i++)
	{
		if (ProcessBlackList[i].Buffer != NULL)
		{
			ExFreePool(ProcessBlackList[i].Buffer);
		}
	}
	ProcessBlackListCnt = 0;
	memset(ProcessBlackList, 0, sizeof(ProcessBlackList));
	KeReleaseSpinLock(&ProcessBlackListLock, irql);
}

VOID InsertBlackListProcess(PUNICODE_STRING Name)
{
	KIRQL irql = { 0 };
	KeAcquireSpinLock(&ProcessBlackListLock, &irql);
	if (ProcessBlackListCnt >= MAX_BLACKLIST_LEN)
	{
		KeReleaseSpinLock(&ProcessBlackListLock, irql);
		return;
	}
	ProcessBlackList[ProcessBlackListCnt].Length = 0;
	ProcessBlackList[ProcessBlackListCnt].MaximumLength = 260;
	ProcessBlackList[ProcessBlackListCnt].Buffer = ExAllocatePool(NonPagedPool, 260);
	if (ProcessBlackList[ProcessBlackListCnt].Buffer == NULL)
	{
		KeReleaseSpinLock(&ProcessBlackListLock, irql);
		return;
	}
	RtlCopyUnicodeString(&ProcessBlackList[ProcessBlackListCnt], Name);
	ProcessBlackListCnt++;
	KeReleaseSpinLock(&ProcessBlackListLock, irql);
}

// Ĭ�ϻص������������Ƿ��سɹ�
NTSTATUS DefDispatch(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, 0);
	return STATUS_SUCCESS;
}

NTSTATUS Dispatch(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
)
{
	PIO_STACK_LOCATION io = IoGetCurrentIrpStackLocation(Irp); // ��ȡ��ǰ�豸ջ�ռ�
	// ����������Զ���Ŀ����룬��ȥ��������ֻ��һ�ֿ����룬���������������Լ����ƣ�����������΢��
	if (io->Parameters.DeviceIoControl.IoControlCode == MY_CTL_CODE)
	{
		// �����֪��R3��ʲô��ʽ��������Ҳ������������������ͨ�ô���
		PCMD pcmd = NULL;
		if (Irp->MdlAddress != NULL)
		{
			pcmd = (PCMD)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
		}
		else
		{
			pcmd = (PCMD)Irp->UserBuffer;
		}
		if (pcmd == NULL)
		{
			pcmd = (PCMD)Irp->AssociatedIrp.SystemBuffer;
		}

		// ʹ���Զ���ͨ�Ű���CMD�и����Ŀ�����
		switch (pcmd->code)
		{
		case KILL_PROCESS:
		{
			PPROCESSINFO info = (PPROCESSINFO)pcmd->in;

			if (info->KillSameName)
			{
				for (int i = 0; i < info->Cnt; i++)
				{
					DbgPrintEx(77, 0, "ɱPID: %lld\r\n", info->Pids[i]);
					PEPROCESS ProcessToKill = NULL;
					NTSTATUS st = PsLookupProcessByProcessId((HANDLE)info->Pids[i], &ProcessToKill);
					if (!NT_SUCCESS(st))
					{
						continue;
					}
					// �ų�������״̬�Ľ���
					if (PsGetProcessExitStatus(ProcessToKill) != STATUS_PENDING)
					{
						ObDereferenceObject(ProcessToKill);
						continue;
					}
					// ��ȡBaseName
					UNICODE_STRING BaseName = { 0 };
					EzGetProcessBaseName(ProcessToKill, &BaseName);
					ObDereferenceObject(ProcessToKill);
					EzEnumProcess(&BaseName, KillProcessCallback, NULL);
					if (info->NoReopen)
					{
						InsertBlackListProcess(&BaseName);
					}		
					RtlFreeUnicodeString(&BaseName);
				}
			}
			else
			{
				for (int i = 0; i < info->Cnt; i++)
				{
					DbgPrintEx(77, 0, "ɱPID: %lld\r\n", info->Pids[i]);
					PEPROCESS ProcessToKill = NULL;
					NTSTATUS st = PsLookupProcessByProcessId((HANDLE)info->Pids[i], &ProcessToKill);
					if (!NT_SUCCESS(st))
					{
						continue;
					}
					// �ų�������״̬�Ľ���
					if (PsGetProcessExitStatus(ProcessToKill) != STATUS_PENDING)
					{
						ObDereferenceObject(ProcessToKill);
						continue;
					}
					EzEnumThread(ProcessToKill, KillThreadCallback, NULL);
				}
			}
			break;
		}
		}
	}
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, 0);
	return STATUS_SUCCESS;
}

// �����̻߳ص����ҵ�һ��ɱһ��
VOID KillThreadCallback(IN PETHREAD Thread, IN PVOID Context)
{
	EzPspTerminateThreadByPointer(Thread, 0, TRUE);
}

// �������̻ص�����һ��ɱһ��
VOID KillProcessCallback(IN PEPROCESS Process, IN PVOID Context)
{
	EzEnumThread(Process, KillThreadCallback, NULL);
}

// ���̴����ص�
VOID CreateProcessCallback(HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create)
{
	if (Create)
	{
		PEPROCESS Process = NULL;
		NTSTATUS st = PsLookupProcessByProcessId(ProcessId, &Process);
		if (!NT_SUCCESS(st))
		{
			return;
		}
		UNICODE_STRING BaseName = { 0 };
		st = EzGetProcessBaseName(Process, &BaseName);
		if (NT_SUCCESS(st))
		{
			DbgPrintEx(77, 0, "��������: %wZ\r\n", &BaseName);
			KIRQL irql = { 0 };
			KeAcquireSpinLock(&ProcessBlackListLock, &irql);
			for (int i = 0; i < ProcessBlackListCnt; i++)
			{
				if (RtlCompareUnicodeString(&BaseName, &ProcessBlackList[i], TRUE) == 0)
				{
					EzEnumThread(Process, KillThreadCallback, NULL);
				}
			}
			KeReleaseSpinLock(&ProcessBlackListLock, irql);
			RtlFreeUnicodeString(&BaseName);
		}
		ObDereferenceObject(Process);
	}
}

VOID Unload(PDRIVER_OBJECT pDriver)
{
	if (pDriver->DeviceObject)
	{
		UNICODE_STRING UnSymName = { 0 };
		RtlInitUnicodeString(&UnSymName, _SYM_NAME);
		IoDeleteSymbolicLink(&UnSymName);

		IoDeleteDevice(pDriver->DeviceObject);
	}

	EzGetNtFunction(NULL, NULL, TRUE);
	ClearProcessBlackList();
	PsSetCreateProcessNotifyRoutine(CreateProcessCallback, TRUE);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pReg)
{

	//PCSTR TQNames[] = {
	//	"DlpAppData.exe",
	//	"TQClient.exe",
	//	"TQDefender.exe",
	//	"TQTray.exe",
	//	"TQUpdate.exe",
	//	"TQSafeUI.exe",
	//	"NACLdis.exe",
	//};

	NTSTATUS status;
	status = PsSetCreateProcessNotifyRoutine(CreateProcessCallback, FALSE);
	if (!NT_SUCCESS(status))
	{
		DbgPrintEx(77, 0, "�������̻ص�ʧ��: %x\r\n", status);
		return status;
	}

	UNICODE_STRING UnDeviceName = { 0 };
	RtlInitUnicodeString(&UnDeviceName, _DEVICE_NAME);

	UNICODE_STRING UnSymName = { 0 };
	RtlInitUnicodeString(&UnSymName, _SYM_NAME);
	//��һ�������豸����
	PDEVICE_OBJECT pDevice = NULL;
	NTSTATUS st = IoCreateDevice(pDriver, 0, &UnDeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDevice);

	if (!NT_SUCCESS(st))
	{
		return STATUS_UNSUCCESSFUL;
	}

	//�ڶ���������������
	st = IoCreateSymbolicLink(&UnSymName, &UnDeviceName);

	if (!NT_SUCCESS(st))
	{
		IoDeleteDevice(pDevice);
		return STATUS_UNSUCCESSFUL;
	}

	//�����������豸��־  ȥ����ʼ����
	pDevice->Flags &= ~DO_DEVICE_INITIALIZING;

	//���Ĳ� ʹ��ʲô�ڴ�
	pDevice->Flags |= DO_BUFFERED_IO;

	pDriver->MajorFunction[IRP_MJ_CREATE] = DefDispatch;
	pDriver->MajorFunction[IRP_MJ_CLOSE] = DefDispatch;
	pDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Dispatch;


	pDriver->DriverUnload = Unload;
	return STATUS_SUCCESS;
}
