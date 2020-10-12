#include "xfdlg.h"
#include "ui_xfdlg.h"
#include <stdlib.h>

using namespace std;

void xfDlg::createStandardPathItems()
{
    if (!_data._dataValid) return;
    QPen pen(Qt::white);
    pen.setWidth(3);

    if (_leftLobeVis.pLobePathItem) delete _leftLobeVis.pLobePathItem;_leftLobeVis.pLobePathItem=NULL;
    if (_rightLobeVis.pLobePathItem) delete _rightLobeVis.pLobePathItem;_rightLobeVis.pLobePathItem=NULL;

    if (!_leftLobeVis.pLobePathItem)
    {
        _leftLobeVis.pLobePathItem= new QGraphicsPathItem(0);
        _leftLobeVis.pLobePathItem->setFlags(QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemIsSelectable);
        _leftLobeVis.pLobePathItem->setZValue(1);
        _leftLobeVis.pLobeGroupItem = new QGraphicsItemGroup(0);
        _leftLobeVis.pLobeGroupItem->addToGroup(_leftLobeVis.pLobePathItem);
        _leftLobeVis.pLobeGroupItem->setFlags(QGraphicsItem::ItemIsMovable);

        _leftLobeVis._nodes.clear();
        _leftLobeVis._nodes.append(XLFLungLobeControlNode(&_leftLobeVis,QPoint(0,0),XLFLLCNT_POINT));
        _leftLobeVis._nodes.append(XLFLungLobeControlNode(&_leftLobeVis,QPoint(20,40),XLFLLCNT_VIRTUAL_POINT));
        _leftLobeVis._nodes.append(XLFLungLobeControlNode(&_leftLobeVis,QPoint(75,25),XLFLLCNT_VIRTUAL_POINT));
        _leftLobeVis._nodes.append(XLFLungLobeControlNode(&_leftLobeVis,QPoint(80,25),XLFLLCNT_POINT));
        _leftLobeVis._nodes.append(XLFLungLobeControlNode(&_leftLobeVis,QPoint(80,35),XLFLLCNT_VIRTUAL_POINT));
        _leftLobeVis._nodes.append(XLFLungLobeControlNode(&_leftLobeVis,QPoint(40,10),XLFLLCNT_VIRTUAL_POINT));
        _leftLobeVis._nodes.append(XLFLungLobeControlNode(&_leftLobeVis,QPoint(45,130),XLFLLCNT_POINT));
        _leftLobeVis._nodes.append(XLFLungLobeControlNode(&_leftLobeVis,QPoint(30,120),XLFLLCNT_VIRTUAL_POINT));
        _leftLobeVis._nodes.append(XLFLungLobeControlNode(&_leftLobeVis,QPoint(20,100),XLFLLCNT_VIRTUAL_POINT));

        _leftLobeVis.pLabelItem = new QGraphicsTextItem("L",_leftLobeVis.pLobePathItem);
        _leftLobeVis.pLabelItem->setFont(font());

        for (int i=0;i<_leftLobeVis._nodes.count();++i)
        {
            ui->pGV->scene()->addItem(_leftLobeVis._nodes.at(i).pVisItem);
            _leftLobeVis.pLobeGroupItem->addToGroup(_leftLobeVis._nodes.at(i).pVisItem);
            _leftLobeVis._nodes.at(i).pVisItem->setParentItem(_leftLobeVis.pLobePathItem);
        }

        ui->pGV->scene()->addItem(_leftLobeVis.pLobeGroupItem);
        _leftLobeVis.pLobeGroupItem->moveBy(154,150);
    }
    _leftLobeVis.updatePath();
    _leftLobeVis.pLobePathItem->setPen(pen);
    if (_leftLobeVis.pLobeGroupItem)
    {
        ui->pGV->scene()->destroyItemGroup(_leftLobeVis.pLobeGroupItem);
        _leftLobeVis.pLobeGroupItem=NULL;
    }

    if (!_rightLobeVis.pLobePathItem)
    {
        _rightLobeVis.pLobePathItem = new QGraphicsPathItem(0);
        _rightLobeVis.pLobePathItem->setFlags(QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemIsSelectable);
        _rightLobeVis.pLobePathItem->setZValue(1);
        _rightLobeVis.pLobeGroupItem = new QGraphicsItemGroup(0);
        _rightLobeVis.pLobeGroupItem->addToGroup(_rightLobeVis.pLobePathItem);
        _rightLobeVis.pLobeGroupItem->setFlags(QGraphicsItem::ItemIsMovable);

        _rightLobeVis._nodes.clear();
        _rightLobeVis._nodes.append(XLFLungLobeControlNode(&_rightLobeVis,QPoint(0,0),XLFLLCNT_POINT));
        _rightLobeVis._nodes.append(XLFLungLobeControlNode(&_rightLobeVis,QPoint(20,10),XLFLLCNT_VIRTUAL_POINT));
        _rightLobeVis._nodes.append(XLFLungLobeControlNode(&_rightLobeVis,QPoint(60,10),XLFLLCNT_VIRTUAL_POINT));
        _rightLobeVis._nodes.append(XLFLungLobeControlNode(&_rightLobeVis,QPoint(75,-25),XLFLLCNT_POINT));
        _rightLobeVis._nodes.append(XLFLungLobeControlNode(&_rightLobeVis,QPoint(80,-10),XLFLLCNT_VIRTUAL_POINT));
        _rightLobeVis._nodes.append(XLFLungLobeControlNode(&_rightLobeVis,QPoint(80,90),XLFLLCNT_VIRTUAL_POINT));
        _rightLobeVis._nodes.append(XLFLungLobeControlNode(&_rightLobeVis,QPoint(30,105),XLFLLCNT_POINT));
        _rightLobeVis._nodes.append(XLFLungLobeControlNode(&_rightLobeVis,QPoint(40,80),XLFLLCNT_VIRTUAL_POINT));
        _rightLobeVis._nodes.append(XLFLungLobeControlNode(&_rightLobeVis,QPoint(20,10),XLFLLCNT_VIRTUAL_POINT));

        _rightLobeVis.pLabelItem = new QGraphicsTextItem("R",_rightLobeVis.pLobePathItem);
        _rightLobeVis.pLabelItem->setFont(font());

        for (int i=0;i<_rightLobeVis._nodes.count();++i)
        {
            ui->pGV->scene()->addItem(_rightLobeVis._nodes.at(i).pVisItem);
            _rightLobeVis.pLobeGroupItem->addToGroup(_rightLobeVis._nodes.at(i).pVisItem);
            _rightLobeVis._nodes.at(i).pVisItem->setParentItem(_rightLobeVis.pLobePathItem);
        }

        ui->pGV->scene()->addItem(_rightLobeVis.pLobeGroupItem);
        _rightLobeVis.pLobeGroupItem->moveBy(300,174);
    }

    _rightLobeVis.updatePath();
    _rightLobeVis.pLobePathItem->setPen(pen);
    if (_rightLobeVis.pLobeGroupItem) {
        ui->pGV->scene()->destroyItemGroup(_rightLobeVis.pLobeGroupItem);
        _rightLobeVis.pLobeGroupItem=NULL;
    }

    hideShowControlPoints(_controlPointsVisible);
}

void xfDlg::hideShowControlPoints(bool b)
{
    for (int i=0;i<_leftLobeVis._nodes.count();++i)
        _leftLobeVis._nodes.at(i).pVisItem->setVisible(b);
    for (int i=0;i<_rightLobeVis._nodes.count();++i)
        _rightLobeVis._nodes.at(i).pVisItem->setVisible(b);
    _controlPointsVisible = b;
}

void xfDlg::hideShowControlButton()
{
    if (ui->pMirrorXTB->isVisible())
    {
        ui->pMirrorXTB->hide();
        ui->pMirrorYTB->hide();
        ui->pHideShowControlPointsTB->hide();
    }
    else
    {
        ui->pMirrorXTB->show();
        ui->pMirrorYTB->show();
        ui->pHideShowControlPointsTB->show();
    }
}

void xfDlg::displayFrame(int frameNr)
{
    if (!pPixItem)
    {
        pPixItem = new xfPixmapItem();
        connect(pPixItem,SIGNAL(doubleClicked(QPointF)),this,SLOT(hideShowControlButton()));
        ui->pGV->scene()->addItem(pPixItem);
    }
    QPixmap pix = QPixmap::fromImage(readTIFFrame(frameNr));
    // overlay info painter
    pPixItem->setPixmap(pix);
    float scale = min((float)pix.width()/(float)ui->pGV->width(),(float)pix.height()/float(ui->pGV->height()))*0.9;
    pPixItem->resetTransform();
    //pPixItem->setTransformOriginPoint(QPointF((float)pix.width()/(float)ui->pGV->width()*0.45,(float)pix.width()/(float)ui->pGV->height()*0.45));
    pPixItem->setScale(scale);

    QTransform mirror;
    float m11 = 1;
    float m22 = 1;
    if (ui->pMirrorXTB->isChecked()) m11=-1;
    if (ui->pMirrorYTB->isChecked()) m22=-1;
    mirror.setMatrix(m11,0.0,0.0,0.0,m22,0.0,0,0,1.0);

    QPointF p(-pix.width()/2*0.9,-pix.height()/2*0.9);

    QTransform TT1(1,0,0,0,1,0,-p.x(),-p.y(),1);
    QTransform TT2(1,0,0,0,1,0,p.x(),p.y(),1);

    pPixItem->setTransform(TT1,true);
    pPixItem->setTransform(mirror,true);
    pPixItem->setTransform(TT2,true);

    //pPixItem->setPos(pPixItem->mapFromScene(ui->pGV->scene()->sceneRect().center())-QPointF(-(float)pix.width()*0.5,-(float)pix.height()*0.5));

    if (!p3DFrameItem)
    {
        p3DFrameItem = new QGraphicsRectItem(QRect(0,0,100,60),pPixItem);
        QPen pen(Qt::white);
        pen.setWidth(3);
        p3DFrameItem->setPen(pen);
        p3DFrameItem->setFlag(QGraphicsItem::ItemIsMovable,true);
        QPixmap button(":/images/small_button.png");
        if (!pCornerPixmapItem) pCornerPixmapItem = new xf3DFrameCornerItem(button,p3DFrameItem,this);
        p3DFrameItem->moveBy(pix.width()/2-50,pix.height()/2-30);
        p3DFrameItem->setZValue(10);
        pCornerPixmapItem->setFlag(QGraphicsItem::ItemIsMovable,true);
        pCornerPixmapItem->setZValue(20);
        pCornerPixmapItem->moveBy(-button.width()/2,-button.height()/2);
        p3DFrameItem->setVisible(true);
        pCornerPixmapItem->setVisible(true);
    }

    if (_data._dataValid)
    {
        if (_data._rotation)
        {
            p3DFrameItem->setVisible(true);
            pCornerPixmapItem->setVisible(true);
            _leftLobeVis.setVisible(false);
            _rightLobeVis.setVisible(false);
        }
        else
        {
            p3DFrameItem->setVisible(false);
            pCornerPixmapItem->setVisible(false);
            _leftLobeVis.setVisible(true);
            _rightLobeVis.setVisible(true);
        }
    }
    else
    {
        if (p3DFrameItem) p3DFrameItem->setVisible(false);
        if (pCornerPixmapItem) pCornerPixmapItem->setVisible(false);
        _leftLobeVis.setVisible(false);
        _rightLobeVis.setVisible(false);
    }
}
