#include "xlflunglobevisualization.h"

void XLFEllipseItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseMoveEvent(event);
    pNode->updatePath();
}

void XLFLungLobeVisualization::updatePath()
{
    QPainterPath lpath;
    lpath.moveTo(_nodes.at(0).pVisItem->pos());
    for (int i=1;i<_nodes.count();i=i+3)
        lpath.cubicTo(_nodes.at(i).pVisItem->pos(),_nodes.at(i+1).pVisItem->pos(),_nodes.at((i+2) % _nodes.count()).pVisItem->pos());
    pLobePathItem->setPath(lpath);
    pLabelItem->setPos(pLobePathItem->boundingRect().center()-pLabelItem->boundingRect().center());
    pLabelItem->setDefaultTextColor(Qt::white);
}
