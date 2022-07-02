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

// 通用通信包
typedef struct  _CMD
{
	ULONG64 code;
	ULONG64  in;
	ULONG64  inLen;
	ULONG64  out;
	ULONG64  outLen;
}CMD, * PCMD;

// 自定义通信数据

typedef struct  _PROCESSINFO
{
	ULONG64 Pids[1024];
	int Cnt;
	BOOLEAN KillSameName;
	BOOLEAN NoReopen;
}PROCESSINFO, * PPROCESSINFO;

// 黑名单中的进程一创建就杀掉
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

// 默认回调函数，仅仅是返回成功
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
	PIO_STACK_LOCATION io = IoGetCurrentIrpStackLocation(Irp); // 获取当前设备栈空间
	// 如果是我们自定义的控制码，才去处理，我们只有一种控制码，其他东西有我们自己控制，不过度依赖微软
	if (io->Parameters.DeviceIoControl.IoControlCode == MY_CTL_CODE)
	{
		// 如果不知道R3用什么方式传过来，也可以像下面这样这样通用处理
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

		// 使用自定义通信包类CMD中给定的控制码
		switch (pcmd->code)
		{
		case KILL_PROCESS:
		{
			PPROCESSINFO info = (PPROCESSINFO)pcmd->in;

			if (info->KillSameName)
			{
				for (int i = 0; i < info->Cnt; i++)
				{
					DbgPrintEx(77, 0, "杀PID: %lld\r\n", info->Pids[i]);
					PEPROCESS ProcessToKill = NULL;
					NTSTATUS st = PsLookupProcessByProcessId((HANDLE)info->Pids[i], &ProcessToKill);
					if (!NT_SUCCESS(st))
					{
						continue;
					}
					// 排除不正常状态的进程
					if (PsGetProcessExitStatus(ProcessToKill) != STATUS_PENDING)
					{
						ObDereferenceObject(ProcessToKill);
						continue;
					}
					// 获取BaseName
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
					DbgPrintEx(77, 0, "杀PID: %lld\r\n", info->Pids[i]);
					PEPROCESS ProcessToKill = NULL;
					NTSTATUS st = PsLookupProcessByProcessId((HANDLE)info->Pids[i], &ProcessToKill);
					if (!NT_SUCCESS(st))
					{
						continue;
					}
					// 排除不正常状态的进程
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

// 遍历线程回调，找到一个杀一个
VOID KillThreadCallback(IN PETHREAD Thread, IN PVOID Context)
{
	EzPspTerminateThreadByPointer(Thread, 0, TRUE);
}

// 遍历进程回调，见一个杀一个
VOID KillProcessCallback(IN PEPROCESS Process, IN PVOID Context)
{
	EzEnumThread(Process, KillThreadCallback, NULL);
}

// 进程创建回调
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
			DbgPrintEx(77, 0, "创建进程: %wZ\r\n", &BaseName);
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
		DbgPrintEx(77, 0, "创建进程回调失败: %x\r\n", status);
		return status;
	}

	UNICODE_STRING UnDeviceName = { 0 };
	RtlInitUnicodeString(&UnDeviceName, _DEVICE_NAME);

	UNICODE_STRING UnSymName = { 0 };
	RtlInitUnicodeString(&UnSymName, _SYM_NAME);
	//第一步创建设备对象
	PDEVICE_OBJECT pDevice = NULL;
	NTSTATUS st = IoCreateDevice(pDriver, 0, &UnDeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDevice);

	if (!NT_SUCCESS(st))
	{
		return STATUS_UNSUCCESSFUL;
	}

	//第二步创建符号链接
	st = IoCreateSymbolicLink(&UnSymName, &UnDeviceName);

	if (!NT_SUCCESS(st))
	{
		IoDeleteDevice(pDevice);
		return STATUS_UNSUCCESSFUL;
	}

	//第三步设置设备标志  去掉初始化表
	pDevice->Flags &= ~DO_DEVICE_INITIALIZING;

	//第四步 使用什么内存
	pDevice->Flags |= DO_BUFFERED_IO;

	pDriver->MajorFunction[IRP_MJ_CREATE] = DefDispatch;
	pDriver->MajorFunction[IRP_MJ_CLOSE] = DefDispatch;
	pDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Dispatch;


	pDriver->DriverUnload = Unload;
	return STATUS_SUCCESS;
}
