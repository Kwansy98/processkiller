#include "dialog.h"
#include "ui_dialog.h"

#include <QTimer>
#include <windows.h>
#include <tlhelp32.h>
#include <QMessageBox>
#include <QSystemTrayIcon>

#define _SYM_NAME "\\\\.\\processkiller"

#define CODE_CTR_INDEX 0x800
#define MY_CTL_CODE CTL_CODE(FILE_DEVICE_UNKNOWN,CODE_CTR_INDEX,METHOD_NEITHER,FILE_ANY_ACCESS)

#define KILL_PROCESS 1

typedef struct  _CMD
{
    ULONG64 code;
    ULONG64  in;
    ULONG64  inLen;
    ULONG64  out;
    ULONG64  outLen;
}CMD, * PCMD;

typedef struct  _PROCESSINFO
{
    ULONG64 Pids[1024];
    int Cnt;
    BOOLEAN KillSameName;
    BOOLEAN NoReopen;
}PROCESSINFO, * PPROCESSINFO;

bool LoadDriver(std::string path, std::string serviceName)
{
    bool bRet = false;
    DWORD dwLastError;
    SC_HANDLE hSCManager;
    SC_HANDLE hService = NULL;

    if (hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS))
    {
        hService = CreateServiceA(
                    hSCManager, serviceName.c_str(),
                    serviceName.c_str(), SERVICE_ALL_ACCESS,
                    SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
                    SERVICE_ERROR_IGNORE, path.c_str(),
                    NULL, NULL, NULL, NULL, NULL
                    );

        if (hService == NULL)
        {
            hService = OpenServiceA(hSCManager, serviceName.c_str(), SERVICE_ALL_ACCESS);

            if (!hService)
            {
                CloseServiceHandle(hSCManager);
                return false;
            }

        }

        bRet = StartServiceA(hService, 0, NULL);
        if (!bRet)
        {
            dwLastError = GetLastError();
            printf("%d\r\n", dwLastError);
        }

    }

    if (hService)
    {
        CloseServiceHandle(hService);
    }

    if (hSCManager)
    {
        CloseServiceHandle(hSCManager);
    }

    return bRet;
}

bool UnloadDriver(std::string serviceName)
{
    BOOL bRet = FALSE;
    SC_HANDLE hServiceMgr = NULL;
    SC_HANDLE hServiceDDK = NULL;
    SERVICE_STATUS SvrSta;

    do
    {
        //
        // 打开SCM管理器
        //
        hServiceMgr = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);

        if (hServiceMgr == NULL)
        {
            break;
        }

        //
        // 打开驱动所对应的服务
        //
        hServiceDDK = OpenServiceA(hServiceMgr, serviceName.c_str(), SERVICE_ALL_ACCESS);

        if (hServiceDDK == NULL)
        {
            break;
        }

        ControlService(hServiceDDK, SERVICE_CONTROL_STOP, &SvrSta);


        if (DeleteService(hServiceDDK))
        {
            bRet = TRUE;
        }

    } while (FALSE);

    if (hServiceDDK)
    {
        CloseServiceHandle(hServiceDDK);
    }

    if (hServiceMgr)
    {
        CloseServiceHandle(hServiceMgr);
    }

    return bRet;
}

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);

    // 设置窗体最大化和最小化
    Qt::WindowFlags windowFlag  = Qt::Dialog;
    windowFlag                  |= Qt::WindowMinimizeButtonHint;
    windowFlag                  |= Qt::WindowMaximizeButtonHint;
    windowFlag                  |= Qt::WindowCloseButtonHint;

    setWindowFlags(windowFlag);

    // 加载驱动
    char szDriverImagePath[MAX_PATH] = { 0 };
    GetFullPathNameA("ProcessKiller.sys", MAX_PATH, szDriverImagePath, NULL);

    // 加载失败就弹窗
    if (!LoadDriver(szDriverImagePath, "processkiller"))
    {
        QMessageBox msg;
        msg.setText("ERROR");
        msg.setInformativeText("加载驱动失败，请右键管理员运行；如果驱动被杀毒软件删除，请从隔离区中恢复");
        msg.exec();
    }

    // 定时器枚举进程，按照PID排序
    QTimer * timer = new QTimer(this);
    timer->start(1000);
    connect(timer,&QTimer::timeout,[=](){
        ui->allProcesses->clear();
        QStringList processes{};
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(pe32);
        HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
        if(hProcessSnap == INVALID_HANDLE_VALUE)
        {
            return;
        }
        BOOL bMore = Process32First(hProcessSnap,&pe32);
        while(bMore)
        {
            QString s = QString("%1").arg(pe32.th32ProcessID) + " " + QString::fromWCharArray(pe32.szExeFile);
            processes.append(s);
            bMore = Process32Next(hProcessSnap,&pe32);
        }
        // sort
        std::sort(processes.begin(), processes.end(), [](const QString& s1, const QString& s2){
            auto pid1 = s1.split(" ")[0].toULongLong();
            auto pid2 = s2.split(" ")[0].toULongLong();
            return pid1 < pid2;
        });
        ui->allProcesses->addItems(processes);

        CloseHandle(hProcessSnap);
    });
}

Dialog::~Dialog()
{
    // 卸载驱动
    UnloadDriver("processkiller");
    delete ui;
}

// 单击选中进程
void Dialog::on_allProcesses_itemClicked(QListWidgetItem *item)
{
    // 不重复添加
    int len = ui->processToKill->findItems(item->text(),Qt::MatchContains).length();
    if (len == 0)
    {
        ui->processToKill->addItem(item->text());
    }
}

// 单击移除进程
void Dialog::on_processToKill_itemClicked(QListWidgetItem *item)
{
    ui->processToKill->removeItemWidget(item);
    ui->processToKill->takeItem(ui->processToKill->currentIndex().row());
}

// 杀死选中进程PID
void Dialog::on_killSelectedButton_clicked()
{
    HANDLE hDevice = CreateFileA(_SYM_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!hDevice)
    {
        DWORD ErrorCode = GetLastError();
        QMessageBox msg;
        msg.setText("ERROR");
        msg.setInformativeText("open device failed: " + QString::number(ErrorCode));
        msg.exec();
    }
    else
    {
        PROCESSINFO info = { 0 };
        info.Cnt = 0;
        info.KillSameName = 0;

        for (int i = 0; i < ui->processToKill->count(); i++)
        {
            ULONG64 pid = ui->processToKill->item(i)->text().split(" ")[0].toULongLong();
            info.Pids[i] = pid;
            info.Cnt++;
        }

        CMD xxx;
        xxx.code = KILL_PROCESS;
        xxx.in = (ULONG64)&info;
        xxx.inLen = sizeof(PROCESSINFO);
        xxx.out = 0;
        xxx.outLen = 0;

        ULONG retLen = 0;
        BOOL isSuccess = DeviceIoControl(hDevice, MY_CTL_CODE, &xxx, sizeof(CMD), &xxx, sizeof(ULONG_PTR), &retLen, 0);
        if (!isSuccess)
        {
            DWORD ErrorCode = GetLastError();
            QMessageBox msg;
            msg.setText("ERROR");
            msg.setInformativeText("DeviceIoControl failed: " + QString::number(ErrorCode));
            msg.exec();
        }
        CloseHandle(hDevice);
        ui->processToKill->clear();
    }
}

// 杀死选中进程和同名进程
void Dialog::on_killSameNameButton_clicked()
{
    HANDLE hDevice = CreateFileA(_SYM_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!hDevice)
    {
        DWORD ErrorCode = GetLastError();
        QMessageBox msg;
        msg.setText("ERROR");
        msg.setInformativeText("open device failed: " + QString::number(ErrorCode));
        msg.exec();
    }
    else
    {
        PROCESSINFO info = { 0 };
        info.Cnt = 0;
        info.KillSameName = 1;

        for (int i = 0; i < ui->processToKill->count(); i++)
        {
            ULONG64 pid = ui->processToKill->item(i)->text().split(" ")[0].toULongLong();
            info.Pids[i] = pid;
            info.Cnt++;
        }

        CMD xxx;
        xxx.code = KILL_PROCESS;
        xxx.in = (ULONG64)&info;
        xxx.inLen = sizeof(PROCESSINFO);
        xxx.out = 0;
        xxx.outLen = 0;

        ULONG retLen = 0;
        BOOL isSuccess = DeviceIoControl(hDevice, MY_CTL_CODE, &xxx, sizeof(CMD), &xxx, sizeof(ULONG_PTR), &retLen, 0);
        if (!isSuccess)
        {
            DWORD ErrorCode = GetLastError();
            QMessageBox msg;
            msg.setText("ERROR");
            msg.setInformativeText("DeviceIoControl failed: " + QString::number(ErrorCode));
            msg.exec();
        }
        CloseHandle(hDevice);
        ui->processToKill->clear();
    }
}

// 关键字搜索
void Dialog::on_searchButton_clicked()
{
    if(ui->processNameKeyword->text().length()==0)
    {
        return;
    }
    QStringList keywords = ui->processNameKeyword->text().split(",");
    for (auto keyword:keywords)
    {
        auto foundProcesses = ui->allProcesses->findItems(keyword,Qt::MatchContains);
        for (int i = 0; i < foundProcesses.length(); i++)
        {
            QString spid = foundProcesses[i]->text().split(" ")[0];
            QString nopid = foundProcesses[i]->text().replace(spid+" ","");
            if (!nopid.toUpper().contains(keyword.toUpper()))
            {
                continue;
            }
            int len = ui->processToKill->findItems(foundProcesses[i]->text(),Qt::MatchContains).length();
            if (len == 0)
            {
                ui->processToKill->addItem(foundProcesses[i]->text());
            }
        }
    }
}

// clear
void Dialog::on_clearButton_clicked()
{
    ui->processToKill->clear();
}

void Dialog::on_killNoReopenButton_clicked()
{
    HANDLE hDevice = CreateFileA(_SYM_NAME, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!hDevice)
    {
        DWORD ErrorCode = GetLastError();
        QMessageBox msg;
        msg.setText("ERROR");
        msg.setInformativeText("open device failed: " + QString::number(ErrorCode));
        msg.exec();
    }
    else
    {
        PROCESSINFO info = { 0 };
        info.Cnt = 0;
        info.KillSameName = 1;
        info.NoReopen = 1;

        for (int i = 0; i < ui->processToKill->count(); i++)
        {
            ULONG64 pid = ui->processToKill->item(i)->text().split(" ")[0].toULongLong();
            info.Pids[i] = pid;
            info.Cnt++;
        }

        CMD xxx;
        xxx.code = KILL_PROCESS;
        xxx.in = (ULONG64)&info;
        xxx.inLen = sizeof(PROCESSINFO);
        xxx.out = 0;
        xxx.outLen = 0;

        ULONG retLen = 0;
        BOOL isSuccess = DeviceIoControl(hDevice, MY_CTL_CODE, &xxx, sizeof(CMD), &xxx, sizeof(ULONG_PTR), &retLen, 0);
        if (!isSuccess)
        {
            DWORD ErrorCode = GetLastError();
            QMessageBox msg;
            msg.setText("ERROR");
            msg.setInformativeText("DeviceIoControl failed: " + QString::number(ErrorCode));
            msg.exec();
        }
        CloseHandle(hDevice);
        ui->processToKill->clear();
    }
}
