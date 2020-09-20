#include "xfmsgdlg.h"
#include "ui_xfmsgdlg.h"

xfMSGDlg::xfMSGDlg(const QString& txt,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::xfMSGDlg)
{
    ui->setupUi(this);
    ui->pTXTLab->setText(txt);
    setWindowFlag(Qt::FramelessWindowHint,true);
}

xfMSGDlg::~xfMSGDlg()
{
    delete ui;
}
