#include "lcdconnector.h"
#include <QPainter>
#include <QBitmap>
#include "xfinsertnrdlg.h"

LCDConnector::LCDConnector(QLCDNumber *pLCD,QToolButton *pUp,QToolButton *pDown,float lLimit,float uLimit,float tick,float *pRef):QObject()
{
    QPixmap up(":/images/up.png");
    up = up.scaledToWidth(30);
    QPixmap empty(40,40);
    empty.fill(QColor(0,0,0,0));
    QPainter pain(&empty);
    pain.drawPixmap(5,5,up);
    pain.end();
    QBitmap upbmp = empty.createHeuristicMask();
    QPixmap down(":/images/down.png");
    down = down.scaledToWidth(30);
    empty.fill(QColor(0,0,0,0));
    QPainter pain2(&empty);
    pain2.drawPixmap(5,5,down);
    pain2.end();
    QBitmap downbmp = empty.createHeuristicMask();

    pUp->setMask(upbmp);
    pDown->setMask(downbmp);

    _lLimit = lLimit;
    _uLimit = uLimit;
    _tick = tick;
    pReference = pRef;

    pUp->setAutoRepeat(true);
    pDown->setAutoRepeat(true);
    pLCD->display(*pRef);

    pUpBtn=pUp;
    pDownBtn=pDown;
    pNr=pLCD;
    pNr->installEventFilter(this);

    connect(pUp,SIGNAL(clicked()),this,SLOT(up()));
    connect(pDown,SIGNAL(clicked()),this,SLOT(down()));
}

void LCDConnector::setNr(const float& nr)
{
    *pReference=nr;
    pNr->display(nr);
}

bool LCDConnector::eventFilter(QObject *watched, QEvent *event)
{
    if (watched==pNr && event->type()==QEvent::MouseButtonPress)
    {
        xfInsertNrDlg dlg(pNr->value());
        connect(&dlg,SIGNAL(nr(const float&)),this,SLOT(setNr(const float&)));
        dlg.exec();
    }

    return QObject::eventFilter(watched,event);
}

void LCDConnector::up()
{
    float value = *pReference+_tick;
    if (value>=_uLimit) {
        value = _uLimit;
        pUpBtn->setEnabled(false);
    }
    else
        pUpBtn->setEnabled(true);
    if (value<=_lLimit) {
        value = _lLimit;
        pDownBtn->setEnabled(false);
    }
    else
        pDownBtn->setEnabled(true);

    *pReference = value;
    pNr->display(*pReference);
}

void LCDConnector::down()
{
    float value = *pReference-_tick;
    if (value>=_uLimit) {
        value = _uLimit;
        pUpBtn->setEnabled(false);
    }
    else
        pUpBtn->setEnabled(true);
    if (value<=_lLimit) {
        value = _lLimit;
        pDownBtn->setEnabled(false);
    }
    else
        pDownBtn->setEnabled(true);

    *pReference = value;
    pNr->display(*pReference);
}

