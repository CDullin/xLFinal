#include "xfdlg.h"
#include "ui_xfdlg.h"
#include "xf_types.h"
#include "xf_tools.h"
#include "xfprogressdlg.h"
#include "tiffio.h"
#include <stdlib.h>
#include <math.h>
#include "../../3rd_party/Alglib/cpp/src/stdafx.h"
#include "../../3rd_party/Alglib/cpp/src/interpolation.h"
#include "../../3rd_party/Alglib/cpp/src/fasttransforms.h"

#include <QtCharts>

using namespace QtCharts;
using namespace std;
using namespace alglib;

void xfDlg::createVisualizationFor2DResults(XLFParam &_param,const QVector <float>& data, float fps, float& _minValue,float &_maxValue)
{
    _minValue=_maxValue=data.at(0);

   _param.pLSeries = new QLineSeries();
   _param.pLSeries->setPen(QPen(Qt::black));
    for (int i=0;i<data.count();++i)
    {
        (*_param.pLSeries) << QPointF((float)i/fps,data.at(i));
        _minValue = std::min(_minValue,data.at(i));
        _maxValue = std::max(_maxValue,data.at(i));
    }

    QPen pline(Qt::darkGray);
    pline.setStyle(Qt::DashDotLine);
    pline.setWidthF(2);

    QPen pmarker(Qt::white);
    pmarker.setWidth(2);

    int markerSize = 13;

    // both
    _param.pIntervals = new QScatterSeries();
    _param.pIntervals->setName("end expiration");
    _param.pIntervals->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
    _param.pIntervals->setPen(pmarker);
    _param.pIntervals->setBrush(QBrush(Qt::lightGray));
    _param.pIntervals->setMarkerSize(markerSize/2);

    _param.pPeaks = new QScatterSeries();
    _param.pPeaks->setName("end inspiration");
    _param.pPeaks->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    _param.pPeaks->setPen(pmarker);
    _param.pPeaks->setBrush(QBrush(Qt::black));
    _param.pPeaks->setMarkerSize(markerSize);

    _param.pDSeries = new QScatterSeries();
    _param.pDSeries->setName("start expiration phase");
    _param.pDSeries->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
    _param.pDSeries->setPen(pmarker);
    _param.pDSeries->setBrush(QBrush(Qt::darkGray));
    _param.pDSeries->setMarkerSize(markerSize/2);

    for (int i=0;i<_param._peaks.count();++i)
        _param.pPeaks->append((float)(_param._peaks.at(i))/fps,data.at(_param._peaks.at(i)));
    for (int i=0;i<_param._intervals.count();++i)
        _param.pIntervals->append((float)(_param._intervals.at(i))/fps,data.at(_param._intervals.at(i)));
    for (int i=0;i<_param._D.count();++i)
        _param.pDSeries->append((float)(_param._D.at(i))/fps,data.at(_param._D.at(i)));

    _param.pCutOffLine=new QLineSeries();
    _param.pCutOffLine->setName("cut off_both");
    _param.pCutOffLine->setPen(pline);
    (*_param.pCutOffLine) << QPointF(0,_param._cutOff) << QPointF((float)data.count()/fps,_param._cutOff);

}

void xfDlg::calculate2DXLF()
{

    int _minFrame = _data._startFrame;
    int _maxFrame = _data._endFrame;

    // create Regions

    QRectF leftRegionBRect = pPixItem->mapRectFromItem(_leftLobeVis.pLobePathItem,_leftLobeVis.pLobePathItem->boundingRect());
    QRectF rightRegionBRect = pPixItem->mapRectFromItem(_rightLobeVis.pLobePathItem,_rightLobeVis.pLobePathItem->boundingRect());
    QRectF rec = leftRegionBRect.united(rightRegionBRect);
    // pixmap item needs to have identity scene transform
    // walk over all frames and calculate av.air, std.air
    // norm values with background
    // trend correction (keep absolute value)
    // walk over all frames and calculate av.left, av.right, std.left, std.right
    float countL=0;
    float sumLBuffer=0;
    float countR=0;
    float sumRBuffer=0;
    float countB=0;
    float sumBBuffer=0;

    QVector <float> _avBoth,_avLeftLobe,_avRightLobe;

    _abb = false;
    xfProgressDlg dlg(0);
    dlg.setRange(0,_data._frames-1);
    dlg.setValue(0);
    connect(&dlg,SIGNAL(aborted()),this,SLOT(abortCurrentFunction()));
    dlg.show();
    const char* _fileName = _data._fileName.toUtf8().constData();
    TIFF *tif = TIFFOpen(_fileName,"r");
    if (!tif)
    {
        emit MSG("couldN't open TIF-file",true);
        return;
    }

    uint32 imageWidth, imageLength;
    uint32 bitsPerPixel;
    unsigned short resUnit;
    uint32 row;
    uint64 offset;
    tdata_t buf;
    float pixInX;

    TIFFGetField(tif,TIFFTAG_IMAGEWIDTH,&imageWidth);
    TIFFGetField(tif,TIFFTAG_IMAGELENGTH,&imageLength);
    TIFFGetField(tif,TIFFTAG_BITSPERSAMPLE,&bitsPerPixel);
    TIFFGetField(tif,TIFFTAG_XRESOLUTION,&pixInX);
    TIFFGetField(tif,TIFFTAG_RESOLUTIONUNIT,&resUnit);

    quint16* pBuffer=(uint16*)_TIFFmalloc(imageWidth*imageLength*sizeof(uint16));

    QPainterPath _lpath = pPixItem->mapFromItem(_leftLobeVis.pLobePathItem,_leftLobeVis.pLobePathItem->path());
    QPainterPath _rpath = pPixItem->mapFromItem(_rightLobeVis.pLobePathItem,_rightLobeVis.pLobePathItem->path());
    long swx = min(rec.left(),rec.right());
    long ewx = max(rec.left(),rec.right())+1;
    long swy = min(rec.bottom(),rec.top());
    long ewy = max(rec.bottom(),rec.top())+1;

    // generate a mask first!!!!
    unsigned char* mask=(unsigned char*)malloc(imageWidth*imageLength);
    memset(mask,0,imageWidth*imageLength);
    for (int x=swx;x<ewx;++x)
        for (int y=swy;y<ewy;++y)
        {
            if (_lpath.contains(QPointF(x,y)))
                mask[x+y*imageWidth]=1;
            if (_rpath.contains(QPointF(x,y)))
                mask[x+y*imageWidth]=2;
        }

    for (long t=0;t<_data._frames && !_abb ;++t)
    {
        countL=countR=countB=0;
        sumLBuffer=sumRBuffer=sumBBuffer=0.0f;
        dlg.setValue(t);
        // read image
        TIFFSetDirectory(tif,t);

        offset = (uint64)pBuffer;
        buf = _TIFFmalloc(TIFFScanlineSize(tif));
        for (row = 0; row < imageLength; row++)
        {
            TIFFReadScanline(tif, buf, row);
            memcpy((void*)offset,buf,imageWidth*2);
            offset+=imageWidth*2;
        }
        _TIFFfree(buf);

        long pos;
        for (int x=swx;x<ewx;++x)
            for (int y=swy;y<ewy;++y)
            {
                pos = x+y*imageWidth;
                if (mask[pos]==1)
                {
                    countL++;
                    countB++;
                    sumLBuffer+=pBuffer[pos];
                    sumBBuffer+=pBuffer[pos];
                }
                if (mask[pos]==2)
                {
                    countR++;
                    countB++;
                    sumRBuffer+=pBuffer[pos];
                    sumBBuffer+=pBuffer[pos];
                }
            }
        countL>0 ? sumLBuffer/=countL : sumLBuffer=0;
        countR>0 ? sumRBuffer/=countR : sumRBuffer=0;
        countB>0 ? sumBBuffer/=countB : sumBBuffer=0;

        _avBoth.append(sumBBuffer);
        _avLeftLobe.append(sumLBuffer);
        _avRightLobe.append(sumRBuffer);
    }

    free(mask);
    TIFFClose(tif);
    if (_abb) return;

    QFile fs("/home/heimdall/trendline_correction.csv");
    fs.open(QFile::WriteOnly);
    QTextStream t(&fs);

    for (QVector <float>::iterator it=_avBoth.begin();it!=_avBoth.end();++it)
        t << QString("%1\n").arg((*it));

    fs.close();

    // calculate mov.average
    float fps = (float)_data._frames / (*_data.pTotalTime);
    int frame = (*_data.pTrendCorrTimeWindowInMS) / 1000.0f * fps;

    QVector <float> _lBg = adaptiveMovAvFilter(_avLeftLobe,(float)frame/2.0f);
    QVector <float> _rBg = adaptiveMovAvFilter(_avRightLobe,(float)frame/2.0f);
    QVector <float> _bBg = adaptiveMovAvFilter(_avBoth,(float)frame/2.0f);

    QVector <float> _alBg = movAvFilter(_avLeftLobe,(float)frame/2.0f);

    // corrected and smoothed again
    QVector <float> _lVal;
    QVector <float> _rVal;
    QVector <float> _bVal;
    for (int i=0;i<_avLeftLobe.count();++i)
    {
        _lVal.append(_avLeftLobe.at(i)/_lBg.at(i));
        _rVal.append(_avRightLobe.at(i)/_rBg.at(i));
        _bVal.append(_avBoth.at(i)/_bBg.at(i));
    }

    // find peaks
    QVector <int> _lPeaks;
    QVector <int> _rPeaks;
    QVector <int> _bPeaks;

    float _lvl = (*_data.pLevelInPercent);

    findMaxPositions(_lVal,_lPeaks,_minFrame,_maxFrame,_lvl);
    findMaxPositions(_rVal,_rPeaks,_minFrame,_maxFrame,_lvl);
    findMaxPositions(_bVal,_bPeaks,_minFrame,_maxFrame,_lvl);
    _avLeftLobe = _lVal;
    _avRightLobe = _rVal;
    _avBoth = _bVal;
    // calculate starting points list, end point list, peak point list

    _data._left = generateParam(_avLeftLobe,_minFrame,_maxFrame,fps,_lvl,(*_data.pTrendCorrTimeWindowInMS)/1000.0f*fps);
    _data._right = generateParam(_avRightLobe,_minFrame,_maxFrame,fps,_lvl,(*_data.pTrendCorrTimeWindowInMS)/1000.0f*fps);
    _data._both = generateParam(_avBoth,_minFrame,_maxFrame,fps,_lvl,(*_data.pTrendCorrTimeWindowInMS)/1000.0f*fps);

    if (!_data._left._valid || !_data._right._valid || !_data._both._valid)
    {
        emit MSG("at least on region caused an error during calculation - no output generated");
    }

    // calculate heart beat?!?
    float _heartRate = -1.0f;
    real_1d_array x;
    complex_1d_array fx;
    x.setlength(_avLeftLobe.count());
    for (int i=0;i<_avLeftLobe.count();++i)
        x[i]=_avLeftLobe.at(i);
    fftr1d(x, _avLeftLobe.count(), fx);

    // read heartbeat
    // quality of heart rate detection?!?
    float maxFrequency=abscomplex(fx[170/30*fps]);
    float baseline = maxFrequency;
    int maxPos=170/30*fps;
    int maxLen = fx.length()/2;
    for (int i=171/30*fps;i<maxLen;++i)
        if (abscomplex(fx[i])>maxFrequency)
        {
            maxFrequency = abscomplex((fx[i]));
            maxPos = i;
        }

    if ((maxPos!=170/30*fps) && (maxFrequency/baseline>2))
        _heartRate = maxPos/fps*60;       // in bpm

    if (!pResultTxtItem)
    {
        pResultTxtItem = new QGraphicsSimpleTextItem();
        ui->pDataGV->scene()->addItem(pResultTxtItem);
        QFont f=font();
        f.setPixelSize(15);
        pResultTxtItem->setFont(f);
        pResultTxtItem->setPen(QPen(Qt::black));
    }

    QString txt;
    txt+=ui->pFileNameLEdit->text()+"\t";
    txt+=QString("total time=%1s    ").arg(*_data.pTotalTime);
    txt+=QString("body mass=%1g\n").arg(*_data.pBodyMass,0,'f',1);
    txt += "region\t#\tbr[s]\tin[s]\tiso\taniso\n\t\tTflow[au*s]\tAT\ttau[1/s]\thr[bpm]\n";

    // update table
    XLFParam *pParam;
    for (int i=0;i<3;++i)
    {
        switch (i)
        {
        case 0 : pParam = &_data._left;
            txt += "left\t";
            break;
        case 1 : pParam = &_data._right;
            txt += "right\t";
            break;
        case 2 : pParam = &_data._both;
            txt += "both\t";
            break;
        }

        txt+=QString("%1").arg(pParam->_peaks.count())+"  ";
        txt+=QString("%1 ± %2").arg(pParam->_avL,0,'f',3).arg(pParam->_stdL,0,'g',3)+"  ";
        txt+=QString("%1 ± %2").arg(pParam->_tin,0,'f',3).arg(pParam->_stdtin,0,'g',3)+"  ";
        txt+=QString("%1 ± %2").arg(pParam->_Iso,0,'f',3).arg(pParam->_stdIso,0,'g',3)+"  ";
        txt+=QString("%1 ± %2").arg(pParam->_AnIso,0,'f',3).arg(pParam->_stdAnIso,0,'g',3)+"\n\t";
        txt+=QString("%1 ± %2").arg(pParam->_TV,0,'f',6).arg(pParam->_stdTV,0,'g',5)+"  ";
        txt+=QString("%1 ± %2").arg(pParam->_ATrp,0,'f',6).arg(pParam->_stdATrp,0,'g',5)+"  ";
        pParam->_decayRate!=-1 ? txt+=QString("%1 [%2]").arg(pParam->_decayRate,0,'f',3).arg(pParam->_RSquaredDecayRate,0,'f',3)+"  " :
                txt += QString("NaN\t");
        _heartRate!=-1 ? txt+= QString("%1").arg(_heartRate,0,'f',1)+"\n" : txt += "NaN\n";

    }

    pResultTxtItem->setText(txt);

    float minL,maxL,minR,maxR,minB,maxB;

    createVisualizationFor2DResults(_data._left,_avLeftLobe,fps,minL,maxL);
    createVisualizationFor2DResults(_data._right,_avRightLobe,fps,minR,maxR);
    createVisualizationFor2DResults(_data._both,_avBoth,fps,minB,maxB);

    float _minBackground=min(minL,min(minR,minB));
    float _maxBackground=max(maxL,max(maxR,maxB));

    if (pChart) {
        ui->pDataGV->scene()->removeItem(pChart);
        delete pChart;
    }
    pChart = new QChart();

    _data._left.addToChart(pChart);
    _data._right.addToChart(pChart);
    _data._both.addToChart(pChart);

    QLineSeries *_leftBoundary = new QLineSeries();
    QLineSeries *_rightBoundary = new QLineSeries();
    (*_leftBoundary) << QPointF((float)_data._startFrame/fps,_minBackground*0.99) << QPointF((float)_data._startFrame/fps,_maxBackground*1.01);
    (*_leftBoundary) << QPointF((float)_data._endFrame/fps,_maxBackground*1.01) << QPointF((float)_data._endFrame/fps,_minBackground*0.99);
    (*_rightBoundary) << QPointF((float)_data._startFrame/fps,_minBackground*0.99) << QPointF((float)_data._startFrame/fps,_minBackground*0.99);
    (*_rightBoundary) << QPointF((float)_data._endFrame/fps,_minBackground*0.99) << QPointF((float)_data._endFrame/fps,_minBackground*0.99);
    QAreaSeries *pAreaSeries = new QAreaSeries(_leftBoundary,_rightBoundary);
    pAreaSeries->setPen(QPen(Qt::gray));
    pAreaSeries->setBrush(QBrush(QColor(100,100,100,33)));
    pChart->addSeries(pAreaSeries);
    pChart->createDefaultAxes();

    //pChart->setAnimationOptions(QChart::SeriesAnimations);
    pChart->legend()->setEnabled(false);
    pChart->setPlotArea(pChart->geometry());
    pChart->createDefaultAxes();

    pChart->axisX()->setTitleText("time [s]");
    //pChart->axisX()->setRange(0,5.0f);
    pChart->axisY()->setTitleText("relative x-ray transmission");

    QValueAxis *pYValueAxis = dynamic_cast<QValueAxis*>(pChart->axisY());
    pYValueAxis->setRange(pYValueAxis->min()*0.999,pYValueAxis->max()*1.001);
    //if (pOrgCV->chart()) delete pOrgCV->chart();
    //pOrgCV->setChart(pChart);

    pChart->setBackgroundBrush(QBrush(Qt::NoBrush));
    pChart->setAnimationOptions(QChart::SeriesAnimations);
    pChart->legend()->hide();
    //pChart->legend()->setVisible(false);
    pChart->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding));
    pChart->setMinimumSize(ui->pDataGV->width()-20,ui->pDataGV->height()-200);
    ui->pDataGV->scene()->setSceneRect(0,0,ui->pDataGV->width()-20,ui->pDataGV->height()-20);
    ui->pDataGV->scene()->addItem(pChart);
    ui->pTabWdgt->setCurrentIndex(1);

    QFont f=font();
    f.setPixelSize(15);
    pResultTxtItem->setFont(f);
    pResultTxtItem->setPos(50,310);

    displaySeries(ui->pVisCB->currentIndex());
    updateAndDisplayStatus();
}

void xfDlg::displaySeries(int nr)
{
    _data._both.setVisible(false);
    _data._left.setVisible(false);
    _data._right.setVisible(false);
    switch (nr)
    {
    case 0 : // left
        pChart->setTitle("left lobe");
        _data._left.setVisible(true);
    break;
    case 1 : // right
        pChart->setTitle("right lobe");
        _data._right.setVisible(true);
    break;
    case 2 : // both
        pChart->setTitle("entire lung");
        _data._both.setVisible(true);
    break;
    }
}
