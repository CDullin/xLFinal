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

    // calculate mov.average
    float fps = (float)_data._frames / (*_data.pTotalTime);
    int frame = (*_data.pTrendCorrTimeWindowInMS) / 1000.0f * fps;

    // orginal data
    QLineSeries *_oData = new QLineSeries();
    _oData->setName("original data");
    for (int i=0;i<_avLeftLobe.count();++i)
        (*_oData) << QPointF((float)i/fps,_avLeftLobe.at(i));

    QVector <float> _lBg = adaptiveMovAvFilter(_avLeftLobe,(float)frame/2.0f);
    QVector <float> _rBg = adaptiveMovAvFilter(_avRightLobe,(float)frame/2.0f);
    QVector <float> _bBg = adaptiveMovAvFilter(_avBoth,(float)frame/2.0f);

    QVector <float> _alBg = movAvFilter(_avLeftLobe,(float)frame/2.0f);

    // smoothed data
    QLineSeries *_avData = new QLineSeries();
    QLineSeries *_aavData = new QLineSeries();
    _avData->setName("adaptive mov. average");
    _aavData->setName("mov. average");
    for (int i=0;i<_avLeftLobe.count();++i)
    {
        (*_avData) << QPointF((float)i/fps,_lBg.at(i));
        (*_aavData) << QPointF((float)i/fps,_alBg.at(i));
    }
    // corrected and smoothed again
    QVector <float> _lVal;
    QVector <float> _rVal;
    QVector <float> _bVal;
    QLineSeries *_corrected = new QLineSeries();
    _corrected->setName("corrected");
    for (int i=0;i<_avLeftLobe.count();++i)
    {
        _lVal.append(_avLeftLobe.at(i)/_lBg.at(i));
        _rVal.append(_avRightLobe.at(i)/_rBg.at(i));
        _bVal.append(_avBoth.at(i)/_bBg.at(i));
        (*_corrected) << QPointF((float)i/fps,_lVal.at(i));
    }

    frame = (*_data.pPeakCorrTimeWindowInMS) / 1000.0f * fps;
    QVector <float> _flBg = movAvFilter(_lVal,(float)frame/2.0f);
    QVector <float> _frBg = movAvFilter(_rVal,(float)frame/2.0f);
    QVector <float> _fbBg = movAvFilter(_bVal,(float)frame/2.0f);

    // filtered again
    QLineSeries *_filteredCorrected = new QLineSeries();
    _filteredCorrected->setName("filtered peak positions");
    for (int i=0;i<_avLeftLobe.count();++i)
        (*_filteredCorrected) << QPointF((float)i/fps,_flBg.at(i));

    // find peaks
    QVector <int> _lPeaks;
    QVector <int> _rPeaks;
    QVector <int> _bPeaks;

    float _lvl = (*_data.pLevelInPercent);

    findMaxPositions(_flBg,_lPeaks,_minFrame,_maxFrame,_lvl);
    findMaxPositions(_frBg,_rPeaks,_minFrame,_maxFrame,_lvl);
    findMaxPositions(_fbBg,_bPeaks,_minFrame,_maxFrame,_lvl);

    QScatterSeries *pPeaks= new QScatterSeries();
    pPeaks->setName("peaks");
    for (unsigned long i=0;i<_lPeaks.count();++i)
        pPeaks->append((float)(_lPeaks.at(i))/fps,_flBg.at(_lPeaks.at(i)));
/*
    QChart *pChart=new QChart();
    //pChart->setTheme(QChart::ChartThemeBlueCerulean);
    pChart->addSeries(_oData);
    pChart->addSeries(_avData);
    pChart->addSeries(_aavData);
    //pChart->addSeries(_corrected);
    //pChart->addSeries(_filteredCorrected);
    //pChart->addSeries(pPeaks);
    pChart->setAnimationOptions(QChart::SeriesAnimations);
    pChart->legend()->setEnabled(false);
    pChart->createDefaultAxes();
    //pAirCV->setChart(pChart);
*/
   _avLeftLobe = _flBg;
   _avRightLobe = _frBg;
   _avBoth = _fbBg;
    // calculate starting points list, end point list, peak point list

    // idea: 1) detect peaks 2) expand area left till change in curvature 3) expand area right till next detected point 4) remove areas that touch boundaries
    // level function %50 min .. max

    //   XLFParam generateParam(QVector <float> values,int minFrame,int maxFrame,float fps,float lvl, int _width, float _intervalFilter);

    _data._left = generateParam(_avLeftLobe,_minFrame,_maxFrame,fps,_lvl,(*_data.pTrendCorrTimeWindowInMS)/1000.0f*fps);
    _data._right = generateParam(_avRightLobe,_minFrame,_maxFrame,fps,_lvl,(*_data.pTrendCorrTimeWindowInMS)/1000.0f*fps);
    _data._both = generateParam(_avBoth,_minFrame,_maxFrame,fps,_lvl,(*_data.pTrendCorrTimeWindowInMS)/1000.0f*fps);

    if (!_data._left._valid || !_data._right._valid || !_data._both._valid)
    {
        emit MSG("at least on region caused an error during calculation - no output generated");
        //return;
    }

    // calculate heart beat?!?
    float _heartRate = -1.0f;
    real_1d_array x;
    complex_1d_array fx;
    xparams xparam;
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
    {
        _heartRate = maxPos/fps*60;       // in bpm
    }

    QLineSeries *pFftTrafoSeries = new QLineSeries();
    for (int i=1;i<fx.length()/2;++i)
        (*pFftTrafoSeries) << QPointF((float)i/fps,abscomplex(fx[i]));
/*
    pChart=new QChart();
    pChart->addSeries(pFftTrafoSeries);
    pChart->createDefaultAxes();
    //pFrequencyCV->setChart(pChart);
    */
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
    txt += "region\t#\tbr[s]\tin[s]\tiso\taniso\n\t\tEIV\tAT\ttau[1/s]\thr[bpm]\n";

    // update table
    XLFParam *pParam;
    for (int i=0;i<3;++i)
    {
        switch (i)
        {
        case 0 : pParam = &_data._left;
            txt += "left  \t";
            break;
        case 1 : pParam = &_data._right;
            txt += "right \t";
            break;
        case 2 : pParam = &_data._both;
            txt += "both  \t";
            break;
        }

        txt+=QString("%1").arg(pParam->_peaks.count())+"  ";
        txt+=QString("%1 ± %2").arg(pParam->_avL,0,'f',3).arg(pParam->_stdL,0,'f',3)+"  ";
        txt+=QString("%1 ± %2").arg(pParam->_tin,0,'f',3).arg(pParam->_stdtin,0,'f',3)+"  ";
        txt+=QString("%1 ± %2").arg(pParam->_Iso,0,'f',3).arg(pParam->_stdIso,0,'f',3)+"  ";
        txt+=QString("%1 ± %2").arg(pParam->_AnIso,0,'f',3).arg(pParam->_stdAnIso,0,'f',3)+"\n\t";
        txt+=QString("%1 ± %2").arg(pParam->_TV,0,'f',6).arg(pParam->_stdTV,0,'f',5)+"  ";
        txt+=QString("%1 ± %2").arg(pParam->_ATrp,0,'f',6).arg(pParam->_stdATrp,0,'f',5)+"  ";
        pParam->_decayRate!=-1 ? txt+=QString("%1 [%2]").arg(pParam->_decayRate,0,'f',3).arg(pParam->_RSquaredDecayRate,0,'f',3)+"  " :
                txt += QString("NaN\t");
        _heartRate!=-1 ? txt+= QString("%1").arg(_heartRate,0,'f',1)+"\n" : txt += "NaN\n";

    }

    pResultTxtItem->setText(txt);

    float _minBackground=_avLeftLobe.at(0);
    float _maxBackground=_avLeftLobe.at(0);

    _bLSeries=new QLineSeries();
    _bRSeries=new QLineSeries();
    _bBSeries=new QLineSeries();
    for (int i=0;i<_avBoth.count();++i)
    {
        (*_bLSeries) << QPointF((float)i/fps,_avLeftLobe.at(i));
        (*_bRSeries) << QPointF((float)i/fps,_avRightLobe.at(i));
        (*_bBSeries) << QPointF((float)i/fps,_avBoth.at(i));

        _minBackground = std::min(_minBackground,_avLeftLobe.at(i));
        _minBackground = std::min(_minBackground,_avRightLobe.at(i));
        _minBackground = std::min(_minBackground,_avBoth.at(i));

        _maxBackground = std::max(_maxBackground,_avLeftLobe.at(i));
        _maxBackground = std::max(_maxBackground,_avRightLobe.at(i));
        _maxBackground = std::max(_maxBackground,_avBoth.at(i));
    }
    _bLSeries->setName("left");
    _bRSeries->setName("right");
    _bBSeries->setName("both");

    // both
    pBIntervals = new QScatterSeries();
    pBIntervals->setName("i");
    pBPeaks= new QScatterSeries();
    pBPeaks->setName("p");
    pBDSeries= new QScatterSeries();
    pBDSeries->setName("d");
    for (int i=0;i<_data._both._peaks.count();++i)
        pBPeaks->append((float)(_data._both._peaks.at(i))/fps,_avBoth.at(_data._both._peaks.at(i)));
    for (int i=0;i<_data._both._intervals.count();++i)
        pBIntervals->append((float)(_data._both._intervals.at(i))/fps,_avBoth.at(_data._both._intervals.at(i)));
    for (int i=0;i<_data._both._D.count();++i)
        pBDSeries->append((float)(_data._both._D.at(i))/fps,_avBoth.at(_data._both._D.at(i)));
    _cutOffLineB=new QLineSeries();
    _cutOffLineB->setName("cut off_both");
    (*_cutOffLineB) << QPointF(0,_data._both._cutOff) << QPointF((float)_avBoth.count()/fps,_data._both._cutOff);

    pLIntervals = new QScatterSeries();
    pLIntervals->setName("i");
    pLPeaks= new QScatterSeries();
    pLPeaks->setName("p");
    pLDSeries= new QScatterSeries();
    pLDSeries->setName("d");
    for (int i=0;i<_data._left._peaks.count();++i)
        pLPeaks->append((float)(_data._left._peaks.at(i))/fps,_avLeftLobe.at(_data._left._peaks.at(i)));
    for (int i=0;i<_data._left._intervals.count();++i)
        pLIntervals->append((float)(_data._left._intervals.at(i))/fps,_avLeftLobe.at(_data._left._intervals.at(i)));
    for (int i=0;i<_data._left._D.count();++i)
        pLDSeries->append((float)(_data._left._D.at(i))/fps,_avLeftLobe.at(_data._left._D.at(i)));
    _cutOffLineL=new QLineSeries();
    _cutOffLineL->setName("cut off_left");
    (*_cutOffLineL) << QPointF(0,_data._left._cutOff) << QPointF((float)_avLeftLobe.count()/fps,_data._left._cutOff);

    pRIntervals = new QScatterSeries();
    pRIntervals->setName("i");
    pRPeaks= new QScatterSeries();
    pRPeaks->setName("p");
    pRDSeries= new QScatterSeries();
    pRDSeries->setName("d");
    for (int i=0;i<_data._right._peaks.count();++i)
        pRPeaks->append((float)(_data._right._peaks.at(i))/fps,_avRightLobe.at(_data._right._peaks.at(i)));
    for (int i=0;i<_data._right._intervals.count();++i)
        pRIntervals->append((float)(_data._right._intervals.at(i))/fps,_avRightLobe.at(_data._right._intervals.at(i)));
    for (int i=0;i<_data._right._D.count();++i)
        pRDSeries->append((float)(_data._right._D.at(i))/fps,_avRightLobe.at(_data._right._D.at(i)));
    _cutOffLineR=new QLineSeries();
    _cutOffLineR->setName("cut off_right");
    (*_cutOffLineR) << QPointF(0,_data._right._cutOff) << QPointF((float)_avRightLobe.count()/fps,_data._right._cutOff);

    if (pChart) {
        ui->pDataGV->scene()->removeItem(pChart);
        delete pChart;
    }
    pChart = new QChart();

    //pChart->setTheme(QChart::ChartThemeBlueIcy);
    pChart->addSeries(_bLSeries);
    pChart->addSeries(_bRSeries);
    pChart->addSeries(_bBSeries);

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
/*
    XLFCheckBox *pCheckBox = new XLFCheckBox(0);
    pCheckBox->setChecked(false);
    connect(pCheckBox,SIGNAL(toggled(bool)),this,SLOT(setSeriesVisible(bool)));
    ui->pResultsTW->setCellWidget(0,0,pCheckBox);
    pCheckBox = new XLFCheckBox(1);
    pCheckBox->setChecked(false);
    connect(pCheckBox,SIGNAL(toggled(bool)),this,SLOT(setSeriesVisible(bool)));
    ui->pResultsTW->setCellWidget(1,0,pCheckBox);
    pCheckBox = new XLFCheckBox(2);
    pCheckBox->setChecked(true);
    connect(pCheckBox,SIGNAL(toggled(bool)),this,SLOT(setSeriesVisible(bool)));
    ui->pResultsTW->setCellWidget(2,0,pCheckBox);
*/

    pChart->addSeries(pBPeaks);
    pChart->addSeries(pBIntervals);
    pChart->addSeries(pBDSeries);
    pChart->addSeries(_cutOffLineB);
    pChart->addSeries(pLPeaks);
    pChart->addSeries(pLIntervals);
    pChart->addSeries(pLDSeries);
    pChart->addSeries(_cutOffLineL);
    pChart->addSeries(pRPeaks);
    pChart->addSeries(pRIntervals);
    pChart->addSeries(pRDSeries);
    pChart->addSeries(_cutOffLineR);
    pChart->createDefaultAxes();

    _bLSeries->setVisible(false);
    pLIntervals->setVisible(false);
    pLPeaks->setVisible(false);
    pLDSeries->setVisible(false);
    _cutOffLineL->setVisible(false);
    _bRSeries->setVisible(false);
    pRIntervals->setVisible(false);
    pRPeaks->setVisible(false);
    pRDSeries->setVisible(false);
    _cutOffLineR->setVisible(false);

    //pChart->setAnimationOptions(QChart::SeriesAnimations);
    pChart->legend()->setEnabled(false);
    pChart->setPlotArea(pChart->geometry());
    pChart->createDefaultAxes();

    pChart->axisX()->setTitleText("time [s]");
    //pChart->axisX()->setRange(0,5.0f);
    pChart->axisY()->setTitleText("relative x-ray transmission");

    QValueAxis *pYValueAxis = dynamic_cast<QValueAxis*>(pChart->axisY());
    pYValueAxis->setRange(pYValueAxis->min()*0.99,pYValueAxis->max()*1.01);
    //if (pOrgCV->chart()) delete pOrgCV->chart();
    //pOrgCV->setChart(pChart);

    pChart->setBackgroundBrush(QBrush(Qt::NoBrush));
    pChart->setAnimationOptions(QChart::SeriesAnimations);
    //pChart->legend()->setVisible(false);
    pChart->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding));
    pChart->setMinimumSize(ui->pDataGV->width()-20,ui->pDataGV->height()-200);
    ui->pDataGV->scene()->setSceneRect(0,0,ui->pDataGV->width()-20,ui->pDataGV->height()-20);
    ui->pDataGV->scene()->addItem(pChart);
    ui->pTabWdgt->setCurrentIndex(1);
    pResultTxtItem->setPos(50,310);

    updateAndDisplayStatus();
}

void xfDlg::displaySeries(int nr)
{
    switch (nr)
    {
    case 0 : // left
        _bLSeries->setVisible(true);
        pLIntervals->setVisible(true);
        pLPeaks->setVisible(true);
        pLDSeries->setVisible(true);
        _cutOffLineL->setVisible(true);

        _bRSeries->setVisible(false);
        pRIntervals->setVisible(false);
        pRPeaks->setVisible(false);
        pRDSeries->setVisible(false);
        _cutOffLineR->setVisible(false);

        _bBSeries->setVisible(false);
        pBIntervals->setVisible(false);
        pBPeaks->setVisible(false);
        pBDSeries->setVisible(false);
        _cutOffLineB->setVisible(false);
    break;
    case 1 : // right
        _bLSeries->setVisible(false);
        pLIntervals->setVisible(false);
        pLPeaks->setVisible(false);
        pLDSeries->setVisible(false);
        _cutOffLineL->setVisible(false);

        _bRSeries->setVisible(true);
        pRIntervals->setVisible(true);
        pRPeaks->setVisible(true);
        pRDSeries->setVisible(true);
        _cutOffLineR->setVisible(true);

        _bBSeries->setVisible(false);
        pBIntervals->setVisible(false);
        pBPeaks->setVisible(false);
        pBDSeries->setVisible(false);
        _cutOffLineB->setVisible(false);
    break;
    case 2 : // both
        _bLSeries->setVisible(false);
        pLIntervals->setVisible(false);
        pLPeaks->setVisible(false);
        pLDSeries->setVisible(false);
        _cutOffLineL->setVisible(false);

        _bRSeries->setVisible(false);
        pRIntervals->setVisible(false);
        pRPeaks->setVisible(false);
        pRDSeries->setVisible(false);
        _cutOffLineR->setVisible(false);

        _bBSeries->setVisible(true);
        pBIntervals->setVisible(true);
        pBPeaks->setVisible(true);
        pBDSeries->setVisible(true);
        _cutOffLineB->setVisible(true);
    break;


    }
}
