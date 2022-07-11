/********************************************************************************
** Form generated from reading UI file 'dialog.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_Dialog
{
public:
    QHBoxLayout *horizontalLayout_3;
    QHBoxLayout *horizontalLayout;
    QVBoxLayout *verticalLayout_3;
    QLabel *label_2;
    QListWidget *allProcesses;
    QVBoxLayout *verticalLayout_2;
    QLabel *label;
    QListWidget *processToKill;
    QVBoxLayout *verticalLayout;
    QLabel *label_3;
    QPushButton *killSelectedButton;
    QPushButton *killSameNameButton;
    QPushButton *killNoReopenButton;
    QPushButton *clearButton;
    QHBoxLayout *horizontalLayout_2;
    QLineEdit *processNameKeyword;
    QPushButton *searchButton;
    QSpacerItem *verticalSpacer;

    void setupUi(QDialog *Dialog)
    {
        if (Dialog->objectName().isEmpty())
            Dialog->setObjectName(QString::fromUtf8("Dialog"));
        Dialog->resize(929, 524);
        horizontalLayout_3 = new QHBoxLayout(Dialog);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        label_2 = new QLabel(Dialog);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setAlignment(Qt::AlignCenter);

        verticalLayout_3->addWidget(label_2);

        allProcesses = new QListWidget(Dialog);
        allProcesses->setObjectName(QString::fromUtf8("allProcesses"));

        verticalLayout_3->addWidget(allProcesses);


        horizontalLayout->addLayout(verticalLayout_3);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        label = new QLabel(Dialog);
        label->setObjectName(QString::fromUtf8("label"));
        label->setAlignment(Qt::AlignCenter);

        verticalLayout_2->addWidget(label);

        processToKill = new QListWidget(Dialog);
        processToKill->setObjectName(QString::fromUtf8("processToKill"));

        verticalLayout_2->addWidget(processToKill);


        horizontalLayout->addLayout(verticalLayout_2);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        label_3 = new QLabel(Dialog);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(label_3);

        killSelectedButton = new QPushButton(Dialog);
        killSelectedButton->setObjectName(QString::fromUtf8("killSelectedButton"));

        verticalLayout->addWidget(killSelectedButton);

        killSameNameButton = new QPushButton(Dialog);
        killSameNameButton->setObjectName(QString::fromUtf8("killSameNameButton"));

        verticalLayout->addWidget(killSameNameButton);

        killNoReopenButton = new QPushButton(Dialog);
        killNoReopenButton->setObjectName(QString::fromUtf8("killNoReopenButton"));

        verticalLayout->addWidget(killNoReopenButton);

        clearButton = new QPushButton(Dialog);
        clearButton->setObjectName(QString::fromUtf8("clearButton"));

        verticalLayout->addWidget(clearButton);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        processNameKeyword = new QLineEdit(Dialog);
        processNameKeyword->setObjectName(QString::fromUtf8("processNameKeyword"));

        horizontalLayout_2->addWidget(processNameKeyword);

        searchButton = new QPushButton(Dialog);
        searchButton->setObjectName(QString::fromUtf8("searchButton"));

        horizontalLayout_2->addWidget(searchButton);


        verticalLayout->addLayout(horizontalLayout_2);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        horizontalLayout->addLayout(verticalLayout);

        horizontalLayout->setStretch(0, 2);
        horizontalLayout->setStretch(1, 2);
        horizontalLayout->setStretch(2, 1);

        horizontalLayout_3->addLayout(horizontalLayout);


        retranslateUi(Dialog);

        QMetaObject::connectSlotsByName(Dialog);
    } // setupUi

    void retranslateUi(QDialog *Dialog)
    {
        Dialog->setWindowTitle(QCoreApplication::translate("Dialog", "processkiller - https://github.com/Kwansy98/processkiller", nullptr));
        label_2->setText(QCoreApplication::translate("Dialog", "\350\277\233\347\250\213\345\210\227\350\241\250 - \345\215\225\345\207\273\351\200\211\344\270\255", nullptr));
        label->setText(QCoreApplication::translate("Dialog", "\345\276\205\346\235\200\350\277\233\347\250\213 - \345\215\225\345\207\273\347\247\273\351\231\244", nullptr));
        label_3->setText(QCoreApplication::translate("Dialog", "\346\216\247\345\210\266\345\217\260", nullptr));
        killSelectedButton->setText(QCoreApplication::translate("Dialog", "\346\240\271\346\215\256PID\346\235\200\350\277\233\347\250\213", nullptr));
        killSameNameButton->setText(QCoreApplication::translate("Dialog", "\346\240\271\346\215\256\345\220\215\345\255\227\346\235\200\350\277\233\347\250\213", nullptr));
        killNoReopenButton->setText(QCoreApplication::translate("Dialog", "\346\240\271\346\215\256\345\220\215\345\255\227\346\235\200\350\277\233\347\250\213\344\270\224\347\246\201\346\255\242\345\244\215\346\264\273", nullptr));
        clearButton->setText(QCoreApplication::translate("Dialog", "\346\270\205\347\251\272\345\276\205\346\235\200\350\277\233\347\250\213\345\210\227\350\241\250", nullptr));
        searchButton->setText(QCoreApplication::translate("Dialog", "\346\220\234\347\264\242\350\277\233\347\250\213", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Dialog: public Ui_Dialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DIALOG_H
