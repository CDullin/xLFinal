#include "xfquestiondlg.h"
#include "ui_xfquestiondlg.h"

#include <QKeyEvent>

xfQuestionDlg::xfQuestionDlg(const QString& txt,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::xfQuestionDlg)
{
    ui->setupUi(this);
    ui->pMSGLab->setText(txt);
    setWindowFlag(Qt::FramelessWindowHint,true);
}

void xfQuestionDlg::keyPressEvent(QKeyEvent *pKEvent)
{
    if (pKEvent)
    {
        if (pKEvent->key()==Qt::Key_Enter | pKEvent->key()==Qt::Key_Return)
            ui->pAcceptTB->animateClick();
        if (pKEvent->key()==Qt::Key_Escape)
            ui->pCancelTB->animateClick();
    }
    QDialog::keyPressEvent(pKEvent);
}

xfQuestionDlg::~xfQuestionDlg()
{
    delete ui;
}
