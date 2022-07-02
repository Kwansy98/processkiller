#ifndef UAC_H
#define UAC_H
#include <windows.h>
#include <shlobj.h>
#include <QCoreApplication>

// for IsUserAnAdmin()
//#pragma comment (lib, "Shell32.lib")

class UAC
{

public:
    // 以管理员权限启动一个新实例
    // true-启动了新实例
    // false-未启动新实例
    static bool runAsAdmin()
    {

        if (IsUserAnAdmin())
        {

            return false; // 当前程序正以管理员权限运行
        }

        QStringList args = QCoreApplication::arguments(); // 获取命令行参数
        if (args.count() < 2 || args[1] != "runas") // 不带参数或参数不为"runas"时，即直接运行
        {

            // 获取应用程序可执行文件的路径
            QString filePath = QCoreApplication::applicationFilePath();

            // 以管理员权限，执行exe程序
            HINSTANCE ins = ShellExecuteA(nullptr, "runas", filePath.toStdString().c_str(),
                                          "runas", nullptr, SW_SHOWNORMAL);
            if (ins > (HINSTANCE)32)
            {

                return true; // 程序新实例启动成功
            }
        }
        return false;
    }
};


#endif // UAC_H
