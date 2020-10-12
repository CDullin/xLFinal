#ifndef XFDLG_H
#define XFDLG_H

#include <QDialog>
#include <QLCDNumber>
#include <QToolButton>
#include <QGraphicsPixmapItem>
#include <QGraphicsItem>
#include <QGraphicsEllipseItem>
#include <QtCharts/QPolarChart>
#include <QtCharts/QLineSeries>
#include "xf_types.h"
#include "xlflunglobevisualization.h"
#include <QtCharts/QScatterSeries>
#include <QGraphicsSceneMouseEvent>

using namespace QtCharts;

class xfDlg;

QT_BEGIN_NAMESPACE
namespace Ui { class xfDlg; }
QT_END_NAMESPACE

class xfPixmapItem:public QObject,public QGraphicsPixmapItem
{
    Q_OBJECT
public:
    xfPixmapItem():QObject(),QGraphicsPixmapItem()
    {
    }
signals:
    void doubleClicked(QPointF itemPos);
protected:
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override
    {
        emit doubleClicked(event->pos());
    };
};

class xf3DFrameCornerItem:public QGraphicsPixmapItem
{
public:
    xf3DFrameCornerItem(const QPixmap& p,QGraphicsItem* pare,xfDlg *pD):QGraphicsPixmapItem(p,pare)
    {
        pDlg = pD;
    };
protected:
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event)
    {
        QGraphicsItem::mouseMoveEvent(event);
        QGraphicsRectItem *pRItem = dynamic_cast<QGraphicsRectItem*>(parentItem());
        if (pRItem)
        {
            QPointF center = pRItem->rect().center();
            QPointF leftCorner = pos()+QPointF(pixmap().width()/2,pixmap().height()/2);

            QRectF _rect = pRItem->rect();
            _rect.setTopLeft(leftCorner);
            _rect.setBottomRight(QPointF(2*center.x()-leftCorner.x(),2*center.y()-leftCorner.y()));

            pRItem->setRect(_rect);
            pRItem->update();
            //pDlg->rectChanged();
        }
    }

    xfDlg *pDlg;
};

class xfDlg : public QDialog
{
    Q_OBJECT

public:
    xfDlg(QWidget *parent = nullptr);
    ~xfDlg();

    void reject() override;

public slots:
    void MSGSlot(const QString& txt,const bool& error=false);

protected:
    bool eventFilter(QObject *, QEvent *) override;
    void updateAndDisplayStatus();
    QImage readTIFFrame(int);
    void saveMultiFrameTIFF();
    void createStandardPathItems();
    void createVisualizationFor2DResults(XLFParam&,const QVector <float>& data, float fps,float& _minValue,float &_maxValue);

protected slots:
    void updateClock();
    void import();
    void selectedImportFile(const QString&);
    void displayFrame(int);
    void startStopPlayTime(bool);
    void dispNextFrame();
    void rotationModeOnOff(bool);
    void start();
    void calculate3DXLF();
    void calculate2DXLF();
    void abortCurrentFunction();
    void exportResults();
    void hideShowControlPoints(bool);
    void resetStartFrame();
    void resetEndFrame();
    void displaySeries(int);
    void hideShowControlButton();
    void dispFrame();

    void defaultSettings();
    void saveSettings();
    void restoreSettings();
    void LCDModified(int);

signals:
    void MSG(const QString& txt,const bool& error=false);

private:
    Ui::xfDlg *ui;
    bool _abb = false;
    DataContainer _data;
    long _errorMsgCount=0;
    XLFLungLobeVisualization _leftLobeVis,_rightLobeVis;
    bool _controlPointsVisible=false;
    xfPixmapItem *pPixItem = nullptr;
    QGraphicsPixmapItem *pExplanationPixItem=nullptr;
    QGraphicsRectItem *p3DFrameItem = nullptr;
    xf3DFrameCornerItem* pCornerPixmapItem=nullptr;
    QGraphicsSimpleTextItem* pResultTxtItem = nullptr;
    QTimer *pPlayTimer = nullptr;
    QChart* pChart = nullptr;
    QString _lastFileName = "";
};
#endif // XFDLG_H
