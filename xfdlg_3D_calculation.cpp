#include "xfdlg.h"
#include "xf_tools.h"
#include <QtCharts/QLineSeries>
#include "xfprogressdlg.h"
#include "tiffio.h"
#include <QtCharts>
#include "../../3rd_party/Alglib/cpp/src/stdafx.h"
#include "../../3rd_party/Alglib/cpp/src/interpolation.h"
#include "../../3rd_party/Alglib/cpp/src/fasttransforms.h"
#include <stdlib.h>
#include <QGraphicsView>
#include "ui_xfdlg.h"

using namespace alglib;
using namespace QtCharts;
using namespace std;

void xfDlg::calculate3DXLF()
{
    // no data ... go away
    if (!_data._dataValid) return;
    // create angle data
    _data._angles.clear();
    for (long j=0;j<_data._frames;++j)
        _data._angles.append((float)j*(*_data.pRotationAngle)/(float)_data._frames);

    float sumBuffer = 0;
    float count = 0;
    QVector <float> _avBoth;

    QRectF rec=p3DFrameItem->rect();
    QPointF p1=rec.topLeft();
    QPointF p2=rec.bottomRight();
    p1=p3DFrameItem->mapToParent(p1);
    p2=p3DFrameItem->mapToParent(p2);
    rec=QRectF(p1,p2);

    // maybe update
    int _minFrame = 0;
    int _maxFrame = _data._frames-1;

    QLineSeries *_oData = new QLineSeries();
    QLineSeries *_oData2 = new QLineSeries();
    QLineSeries *_linearData = new QLineSeries();
    float fps = (float)_data._frames / (*_data.pTotalTime);

    float _maxValue = 0;
    float _minValue = 100000;

    QImage currentFrame;

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

    for (long t=0;t<_data._frames && !_abb ;++t)
    {
        sumBuffer = 0.0;
        count = 0.0;
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

        for (long x=rec.left();x<rec.right();++x)
            for (long y=rec.top();y<rec.bottom();++y)
            {
                count++;
                sumBuffer+=pBuffer[x+y*imageWidth];
            }
        sumBuffer/=count;
        _maxValue  = std::max(_maxValue,sumBuffer);
        _minValue  = std::min(_minValue,sumBuffer);
        _avBoth.append(sumBuffer);

    }

    TIFFClose(tif);
    if (_abb) return;

    // long term trend correction
    // step 1: moving average
    QVector <float> _trend = movAvFilter(_avBoth,(*_data.pTrendCorrTimeWindowInMS)*fps/1000.0f);
    // step 2: ration to +180 deg vorsicht hier
    int _inc = (_avBoth.count())/2;
    int _size =(_avBoth.count());
    QVector <float> _ratio;
    for (int i=0;i<_avBoth.count()/2-2;++i)
        _ratio.append(_trend.at(i)/_trend.at((i+_inc)%_size));
    // apply to data
    for (int i=0;i<_size;++i)
        i < _ratio.count() ? _avBoth[i]*=_ratio[i] : _avBoth[i]*=_ratio[_ratio.count()-1];

    // forward projection
    complex_1d_array x;
    complex_1d_array fx;
    xparams param;
    x.setlength(_avBoth.count());

    for (int i=0;i<_avBoth.count();++i)
    {
        x[i].x=_avBoth.at(i)*cos(_data._angles[i]*M_PI/180.0f);
        x[i].y=_avBoth.at(i)*sin(_data._angles[i]*M_PI/180.0f);
    }
    fftc1d(x, _avBoth.count(),param);

    _data._heartRateInBPM = -1.0f;
    float maxFrequency=abscomplex(x[170/30*fps]);
    float baseline = maxFrequency;
    int maxPos=170/30*fps;
    int maxLen = x.length()/2;
    for (int i=171/30*fps;i<maxLen;++i)
        if (abscomplex(x[i])>maxFrequency)
        {
            maxFrequency = abscomplex((x[i]));
            maxPos = i;
        }

    if ((maxPos!=170/30*fps) && (maxFrequency/baseline>2))
    {
        _data._heartRateInBPM = maxPos/fps*60;       // in bpm
    }


    //manipulate fourier space
    for (int i=(*_data.pHarmonics);i<_avBoth.count()-(*_data.pHarmonics);++i)
    {
        x[i].x=0;
        x[i].y=0;
    }

    float _level = (*_data.pLevelInPercent)/100.0f;

    // inverse projection
    fftc1dinv(x,_avBoth.count(),param);

    double _maxBackProj=0;
    double _minBackProj=10000;
    for (int i=_minFrame;i<_maxFrame;++i)
    {
        _maxBackProj = std::max(_maxBackProj,_avBoth.at(i)-abscomplex(x[i]));
        _minBackProj = std::min(_minBackProj,_avBoth.at(i)-abscomplex(x[i]));
    }


    QVector <float> _minusBackground;
    float val;
    for (long t=0;t<_data._frames;++t)
    {
        _oData->append(_data._angles[t],_avBoth.at(t)/_maxValue);

        val = ((_avBoth.at(t)-abscomplex(x[t]))-_minBackProj)/(_maxBackProj-_minBackProj);

        _minusBackground << val;
        _oData2->append(_data._angles[t],val);
        _linearData->append(_data._angles[t],val);
        //_linearData->append(_data._angles.at(t),_ratio.at(min(t,(long)_ratio.count()-1)));
    }

    // 20-340°
    _minFrame = (float)_size / 360.0 * 45.0;
    _maxFrame = (float)_size / 360.0 * 315.0;
    QVector <int> _bPeaks;

    int minIntervalFrames = fps * (*_data.pMinIntervalLengthInMS) / 1000.0f;
    findMaxPositions(_minusBackground,_bPeaks,_minFrame,_maxFrame,(*_data.pLevelInPercent),minIntervalFrames);

    QVector<QVector<float>> _intervals;
   // find left and right positions
    for (int p=0;p<_bPeaks.count();++p)
    {
        bool _lFound = false;
        float _lPos;
        long _lPP,_rPP;

        long swt = _bPeaks.at(p);
        long ewt;
        p > 0 ? ewt = _bPeaks.at(p-1) : ewt=0;

        float v2 = _minusBackground.at(swt);
        float v1 = 0;

        for (long j=swt;j>=ewt && !_lFound;j--)
        {
            v1 = _minusBackground.at(j);
            if (v1>v2)
            {
                // left point found
                _lPos = _data._angles[j+1];
                _lPP = j+1;
                _lFound = true;
            }
            v2=v1;
        }

        p < _bPeaks.count()-1 ? ewt = _bPeaks.at(p+1) : ewt = _minusBackground.count()-1;
        bool _rFound = false;
        float _rPos;

        v2 = _minusBackground.at(swt);
        for (long j=swt;j<=ewt && !_rFound;++j)
        {
            v1 = _minusBackground.at(j);
            if (v1>v2)
            {
                // right point found
                _rPos = _data._angles[j-1];
                _rPP = j-1;
                _rFound = true;
            }
            v2=v1;
        }

        if (_lFound && _rFound)
            // left Angle, peak Angle, right Angle
            _intervals.append(QVector<float>() << _lPos << _rPos << _data._angles.at(_bPeaks.at(p)) << _lPP << _rPP);
    }

    // generate interval series
    QScatterSeries *pIntervals = new QScatterSeries();
    pIntervals->setName("intervals");
    pIntervals->setPen(QPen(Qt::black));
    pIntervals->setBrush(QBrush(Qt::blue));
    pIntervals->setMarkerSize(5);
    QScatterSeries *pLIntervals = new QScatterSeries();
    pLIntervals->setName("intervals");
    pLIntervals->setPen(QPen(Qt::black));
    pLIntervals->setBrush(QBrush(Qt::blue));
    pLIntervals->setMarkerSize(5);
    for (int i=0;i<_intervals.count();++i)
    {
/*
        pIntervals->append(_intervals.at(i)[0],_level);
        pIntervals->append(_intervals.at(i)[1],_level);
        pLIntervals->append(_intervals.at(i)[0],_level);
        pLIntervals->append(_intervals.at(i)[1],_level);
*/
        pIntervals->append(_intervals.at(i)[0],_minusBackground[_intervals.at(i)[3]]);
        pIntervals->append(_intervals.at(i)[1],_minusBackground[_intervals.at(i)[4]]);
        pLIntervals->append(_intervals.at(i)[0],_minusBackground[_intervals.at(i)[3]]);
        pLIntervals->append(_intervals.at(i)[1],_minusBackground[_intervals.at(i)[4]]);
    }
    //_data._both = generateParam(_minusBackground,_minFrame,_maxFrame);

    QScatterSeries *pPeaks= new QScatterSeries();
    pPeaks->setName("peaks");
    pPeaks->setPen(QPen(Qt::black));
    pPeaks->setBrush(QBrush(Qt::red));
    pPeaks->setMarkerSize(5);

    QScatterSeries *pLPeaks= new QScatterSeries();
    pLPeaks->setName("peaks");
    pLPeaks->setPen(QPen(Qt::black));
    pLPeaks->setBrush(QBrush(Qt::red));
    pLPeaks->setMarkerSize(5);

    for (unsigned long i=0;i<_bPeaks.count();++i)
    {
        pPeaks->append(_data._angles.at(_bPeaks.at(i)),_minusBackground.at(_bPeaks.at(i)));
        pLPeaks->append(_data._angles.at(_bPeaks.at(i)),_minusBackground.at(_bPeaks.at(i)));
    }

    // calculate L and stdL
    QVector <double> _sum(4,0);
    QVector <double> _quadSum(4,0);

    for (int i=0;i<_intervals.count()-1;++i)
    {
        // L
        _sum[0]+=_intervals.at(i+1)[2]-_intervals.at(i)[2];
        _quadSum[0]+=pow(_intervals.at(i+1)[2]-_intervals.at(i)[2],2);
    }
    _sum[0]/=(double)(_intervals.count()-1);
    _quadSum[0]=_quadSum[0]/(double)(_intervals.count()-1)-pow(_sum[0],2);

    for (int i=0;i<_intervals.count();++i)
    {
        // tin
        _sum[1]+=_intervals.at(i)[2]-_intervals.at(i)[0];
        _quadSum[1]+=pow(_intervals.at(i)[2]-_intervals.at(i)[0],2);

        // iso
        _sum[2]+=(_intervals.at(i)[1]-_intervals.at(i)[0])/(_intervals.at(i)[2]-_intervals.at(i)[0]);
        _quadSum[2]+=pow((_intervals.at(i)[1]-_intervals.at(i)[0])/(_intervals.at(i)[2]-_intervals.at(i)[0]),2);

        // duration
        _sum[3]+=(_intervals.at(i)[1]-_intervals.at(i)[0]);
        _quadSum[3]+=pow((_intervals.at(i)[1]-_intervals.at(i)[0]),2);
    }
    _sum[1]/=(double)(_intervals.count());
    _quadSum[1]=_quadSum[1]/(double)(_intervals.count())-pow(_sum[1],2);
    _sum[2]/=(double)(_intervals.count());
    _quadSum[2]=_quadSum[2]/(double)(_intervals.count())-pow(_sum[2],2);
    _sum[3]/=(double)(_intervals.count());
    _quadSum[3]=_quadSum[3]/(double)(_intervals.count())-pow(_sum[3],2);

    float _totalAngle = (_data._angles.at(_data._angles.count()-1)-_data._angles.at(0));
    float _timePerAngle = (*_data.pTotalTime) / _totalAngle;

    /*
    int i=2;
    QTableWidgetItem *pItem;
    pItem = new QTableWidgetItem(QString("%1").arg(_intervals.count()));
    pItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    ui->pResultsTW->setItem(i,1,pItem);

    pItem = new QTableWidgetItem(QString("%1 ± %2").arg(_sum[0]*_timePerAngle,0,'f',3).arg(_quadSum[0]*_timePerAngle,0,'f',3));
    pItem->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
    ui->pResultsTW->setItem(i,2,pItem);

    pItem = new QTableWidgetItem(QString("%1 ± %2").arg(_sum[1]*_timePerAngle,0,'f',3).arg(_quadSum[1]*_timePerAngle,0,'f',3));
    pItem->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
    ui->pResultsTW->setItem(i,3,pItem);

    pItem = new QTableWidgetItem(QString("%1 ± %2").arg(_sum[2],0,'f',3).arg(_quadSum[2],0,'f',3));
    pItem->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
    ui->pResultsTW->setItem(i,4,pItem);
*/
    if (pChart)
    {
        ui->pDataGV->scene()->removeItem(pChart);
    }

    QLineSeries *pLevel=new QLineSeries();
    pLevel->setPen(QPen(Qt::red));
    pLevel->setName("level");
    for (int i=0;i<37;++i)
        pLevel->append(i*10,_level);
    QPolarChart *pPolChart=new QPolarChart();
    pPolChart->addSeries(_oData);
    _oData->setName("raw data");
    pPolChart->addSeries(_oData2);
    _oData2->setName("filtered breathing events");
    pPolChart->addSeries(pLevel);
    pPolChart->addSeries(pPeaks);
    pPolChart->addSeries(pIntervals);
    pPolChart->legend()->setVisible(false);
    QValueAxis *angularAxis = new QValueAxis();
    angularAxis->setRange(0, 360);
    angularAxis->setTickCount(19);
    angularAxis->setTickInterval(20);
    QValueAxis *radialAxis = new QValueAxis();
    radialAxis->setLabelFormat("%.2f");
    radialAxis->setMax(1.0);
    radialAxis->setMin(0.0);
    pPolChart->addAxis(radialAxis, QPolarChart::PolarOrientationRadial);
    pPolChart->addAxis(angularAxis, QPolarChart::PolarOrientationAngular);
    _oData->attachAxis(angularAxis);
    _oData->attachAxis(radialAxis);
    _oData2->attachAxis(angularAxis);
    _oData2->attachAxis(radialAxis);
    pLevel->attachAxis(angularAxis);
    pLevel->attachAxis(radialAxis);
    pPeaks->attachAxis(angularAxis);
    pPeaks->attachAxis(radialAxis);
    pIntervals->attachAxis(angularAxis);
    pIntervals->attachAxis(radialAxis);
    //pChart->addSeries(_avData);
    pPolChart->setAnimationOptions(QChart::SeriesAnimations);
    pPolChart->legend()->setEnabled(false);


    pPolChart->setBackgroundBrush(QBrush(Qt::NoBrush));
    pPolChart->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding));
    pPolChart->setMinimumSize(ui->pDataGV->height()-20,ui->pDataGV->height()-20);
    ui->pDataGV->scene()->setSceneRect(0,0,ui->pDataGV->width()-20,ui->pDataGV->height()-20);
    pChart = pPolChart;
    ui->pDataGV->scene()->addItem(pChart);
    ui->pTabWdgt->setCurrentIndex(1);


    if (!pResultTxtItem)
    {
        pResultTxtItem = new QGraphicsSimpleTextItem();
        ui->pDataGV->scene()->addItem(pResultTxtItem);
    }

    QString txt;
    txt+=ui->pFileNameLEdit->text()+"\n";
    txt+=QString("total time [s] = \t\t%1\n").arg(*_data.pTotalTime);
    if (_data._rotation) txt+=QString("rotation angle [°] = \t\t%1\n").arg(*_data.pRotationAngle);
    txt+=QString("body mass [g] = \t\t%1\n\n").arg(*_data.pBodyMass,0,'f',1);

    txt+=QString("breathing events = \t\t%1\n").arg(_intervals.count());
    txt+=QString("av. rate [s] = \t\t%1 ± %2\n").arg(_sum[0]*_timePerAngle,0,'f',3).arg(_quadSum[0]*_timePerAngle,0,'f',3);
    txt+=QString("av. duration [s] = \t\t%1 ± %2\n").arg(_sum[3]*_timePerAngle,0,'f',3).arg(_quadSum[3]*_timePerAngle,0,'f',3);
    txt+=QString("inspiration time [s] = \t%1 ± %2\n").arg(_sum[1]*_timePerAngle,0,'f',3).arg(_quadSum[1]*_timePerAngle,0,'f',3);
    txt+=QString("isotrophy index = \t\t%1 ± %2\n").arg(_sum[2],0,'f',3).arg(_quadSum[2],0,'f',3);
    _data._heartRateInBPM!=-1 ? txt+=QString("\nheart rate [bpm] = \t\t%1\n").arg(_data._heartRateInBPM,0,'f',3) : txt+=QString("\nheart rate [bpm] = NaN\n");

    pResultTxtItem->setFont(font());
    pResultTxtItem->setPen(QPen(Qt::black));
    pResultTxtItem->setPos(ui->pDataGV->height()-20,20);
    pResultTxtItem->setText(txt);
    updateAndDisplayStatus();
}
