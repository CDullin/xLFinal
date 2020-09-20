#ifndef LCDCONNECTOR_H
#define LCDCONNECTOR_H

#include <QObject>
#include <QEvent>
#include <QToolButton>
#include <QLCDNumber>

class LCDConnector:QObject
{
Q_OBJECT
public:
    LCDConnector(QLCDNumber *pLCD,QToolButton *pUp,QToolButton *pDown,float lLimit,float uLimit,float tick,float *pRef);

protected slots:
    void up();
    void down();
    void setNr(const float& nr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

    float _lLimit;
    float _uLimit;
    float *pReference;
    float _tick;
    QToolButton *pUpBtn,*pDownBtn;
    QLCDNumber *pNr;
};

#endif // LCDCONNECTOR_H
