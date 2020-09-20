#ifndef XLFLUNGLOBEVISUALIZATION_H
#define XLFLUNGLOBEVISUALIZATION_H

#include <QLabel>
#include <QGraphicsTextItem>
#include <QGraphicsPathItem>
#include <QPen>

class XLFLungLobeVisualization;
class XLF3DRegion;

enum XLFLungLobeControlNodeType
{
    XLFLLCNT_INVALID,
    XLFLLCNT_POINT,
    XLFLLCNT_VIRTUAL_POINT
};

class XLFEllipseItem:public QGraphicsEllipseItem
{
public:
    XLFEllipseItem():QGraphicsEllipseItem(){};
    XLFEllipseItem(XLFLungLobeVisualization* pN,const QRectF &r):QGraphicsEllipseItem(r)
    {
        pNode = pN;
    };
protected:
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    XLFLungLobeVisualization *pNode;
};

class XLFLungLobeControlNode
{
public:
    XLFLungLobeVisualization *pParent=nullptr;
    XLF3DRegion *pParentRegion=nullptr;
    XLFLungLobeControlNodeType _type;
    XLFEllipseItem *pVisItem = nullptr;
    XLFLungLobeControlNode()
    {
        _type = XLFLLCNT_INVALID;
        pVisItem=NULL;
    }
    XLFLungLobeControlNode(XLFLungLobeVisualization *parent,const QPointF& p, const XLFLungLobeControlNodeType& t)
    {
        QPen pen(Qt::white);
        pen.setWidth(3);

        pVisItem=NULL;
        pParent = parent;
        _type = t;
        // create shape, color, overlay cursor
        switch (_type)
        {
        case XLFLLCNT_POINT:
            pVisItem = new XLFEllipseItem(pParent,QRectF(-10,-10,20,20));
            pVisItem->moveBy(p.x(),p.y());
            pVisItem->setBrush(QBrush(QColor(Qt::darkGray)));
            pVisItem->setPen(pen);
            break;
        case XLFLLCNT_VIRTUAL_POINT:
            pVisItem = new XLFEllipseItem(pParent,QRectF(-10,-10,20,20));
            pVisItem->moveBy(p.x(),p.y());
            pVisItem->setBrush(QBrush(Qt::NoBrush));
            pVisItem->setPen(pen);
            break;
        }
        pVisItem->setZValue(2);
        pVisItem->setFlags(QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemIsSelectable);
    }
    void setVisible(bool b)
    {
        if (pVisItem) pVisItem->setVisible(b);
    }
};

class XLFLungLobeVisualization
{
public:
    XLFLungLobeVisualization(){
        pLobePathItem=NULL;
        pLobeGroupItem=NULL;
        pLabelItem=NULL;
    };
    QGraphicsPathItem *pLobePathItem = nullptr;
    QGraphicsItemGroup *pLobeGroupItem = nullptr;
    QGraphicsTextItem *pLabelItem = nullptr;
    QVector <XLFLungLobeControlNode> _nodes;
    void updatePath();
    void setVisible(bool b)
    {
        if (pLobePathItem) pLobePathItem->setVisible(b);
        if (pLobeGroupItem) pLobeGroupItem->setVisible(b);
        if (pLabelItem) pLabelItem->setVisible(b);
        //for (int i=0;i<_nodes.count();++i) _nodes[i].setVisible(b);
    }
};
#endif // XLFLUNGLOBEVISUALIZATION_H
