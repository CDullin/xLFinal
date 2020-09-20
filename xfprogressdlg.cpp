#include "xfprogressdlg.h"
#include "ui_xfprogressdlg.h"

xfProgressDlg::xfProgressDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::xfProgressDlg)
{
    ui->setupUi(this);    
    connect(ui->pEmergencyTB,SIGNAL(clicked()),this,SIGNAL(aborted()));
    setWindowFlag(Qt::FramelessWindowHint,true);
}

xfProgressDlg::~xfProgressDlg()
{
    delete ui;
}


void xfProgressDlg::setRange(int a, int b)
{
    ui->pProgBar->setRange(a,b);
}

void xfProgressDlg::setValue(int v)
{
    ui->pProgBar->setValue(v);
    qApp->processEvents();

}
