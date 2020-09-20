#include "xfinsertnrdlg.h"
#include "ui_xfinsertnrdlg.h"
#include <QKeyEvent>

xfInsertNrDlg::xfInsertNrDlg(const float& nr,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::xfInsertNrDlg)
{
    ui->setupUi(this);
    setWindowFlag(Qt::FramelessWindowHint,true);

    ui->pNr->setText(QString("%1").arg(nr));

    connect(ui->p0,SIGNAL(clicked()),this,SLOT(BtnClicked()));
    connect(ui->p1,SIGNAL(clicked()),this,SLOT(BtnClicked()));
    connect(ui->p2,SIGNAL(clicked()),this,SLOT(BtnClicked()));
    connect(ui->p3,SIGNAL(clicked()),this,SLOT(BtnClicked()));
    connect(ui->p4,SIGNAL(clicked()),this,SLOT(BtnClicked()));
    connect(ui->p5,SIGNAL(clicked()),this,SLOT(BtnClicked()));
    connect(ui->p6,SIGNAL(clicked()),this,SLOT(BtnClicked()));
    connect(ui->p7,SIGNAL(clicked()),this,SLOT(BtnClicked()));
    connect(ui->p8,SIGNAL(clicked()),this,SLOT(BtnClicked()));
    connect(ui->p9,SIGNAL(clicked()),this,SLOT(BtnClicked()));
    connect(ui->pDel,SIGNAL(clicked()),this,SLOT(BtnClicked()));
    connect(ui->pOk,SIGNAL(clicked()),this,SLOT(BtnClicked()));
    connect(ui->pDot,SIGNAL(clicked()),this,SLOT(BtnClicked()));

    installEventFilter(this);
}

bool xfInsertNrDlg::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type()==QEvent::KeyPress)
    {
        QKeyEvent *pKEvent = dynamic_cast<QKeyEvent*>(event);
        if (pKEvent)
        {
            switch (pKEvent->key())
            {
            case Qt::Key_0 : ui->p0->animateClick();break;
            case Qt::Key_1 : ui->p1->animateClick();break;
            case Qt::Key_2 : ui->p2->animateClick();break;
            case Qt::Key_3 : ui->p3->animateClick();break;
            case Qt::Key_4 : ui->p4->animateClick();break;
            case Qt::Key_5 : ui->p5->animateClick();break;
            case Qt::Key_6 : ui->p6->animateClick();break;
            case Qt::Key_7 : ui->p7->animateClick();break;
            case Qt::Key_8 : ui->p8->animateClick();break;
            case Qt::Key_9 : ui->p9->animateClick();break;
            case Qt::Key_periodcentered:
            case Qt::Key_Comma:
            case Qt::Key_Period : ui->pDot->animateClick();break;
            case Qt::Key_Escape : reject();break;
            case Qt::Key_Return :
            case Qt::Key_Enter : ui->pOk->animateClick();break;
            case Qt::Key_Backspace:
            case Qt::Key_Delete : ui->pDel->animateClick();break;
            }
        }
    }
    return QDialog::eventFilter(watched,event);
}

void xfInsertNrDlg::accept()
{
    emit nr(ui->pNr->text().toFloat());
    QDialog::accept();
}

void xfInsertNrDlg::BtnClicked()
{
    QToolButton *pBtn = dynamic_cast<QToolButton*>(sender());
    if (pBtn)
    {
        if (pBtn->text().toLower()=="ok") accept();
        else
        {
            if (pBtn->text().toLower()=="del")
            {
                ui->pNr->setText(ui->pNr->text().left(ui->pNr->text().count()-1));
            }
            else
            {
                ui->pNr->setText(ui->pNr->text()+pBtn->text());
            }
        }
    }
}

xfInsertNrDlg::~xfInsertNrDlg()
{
    delete ui;
}
