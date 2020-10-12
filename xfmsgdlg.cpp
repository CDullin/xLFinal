#include "xfmsgdlg.h"
#include "ui_xfmsgdlg.h"

#include <QKeyEvent>

xfMSGDlg::xfMSGDlg(const QString& txt,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::xfMSGDlg)
{
    ui->setupUi(this);
    ui->pTXTLab->setText(txt);
    setWindowFlag(Qt::FramelessWindowHint,true);
}

void xfMSGDlg::keyPressEvent(QKeyEvent *pKEvent)
{
    if (pKEvent && (pKEvent->key()==Qt::Key_Enter | pKEvent->key()==Qt::Key_Return | pKEvent->key()==Qt::Key_Escape))
        ui->pCancelTB->animateClick();
    return QDialog::keyPressEvent(pKEvent);
}

xfMSGDlg::~xfMSGDlg()
{
    delete ui;
}
