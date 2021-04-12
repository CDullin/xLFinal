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

    QRectF rec=pPixItem->mapRectFromItem(p3DFrameItem,p3DFrameItem->rect());

    // maybe update
    int _minFrame = _data._startFrame;
    int _maxFrame = _data._endFrame;

    _data._both.pLSeries = new QLineSeries();
    _data._both.pOrgData = new QLineSeries();
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

    long swx = min(rec.left(),rec.right());
    long ewx = max(rec.left(),rec.right())+1;
    long swy = min(rec.bottom(),rec.top());
    long ewy = max(rec.bottom(),rec.top())+1;

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

        for (long x=swx;x<ewx;++x)
            for (long y=swy;y<ewy;++y)
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
    complex_1d_array y;
    xparams param;
    x.setlength(_avBoth.count());
    y.setlength(_avBoth.count());

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

    // to check the harmonics
    for (int i=0;i<_avBoth.count();++i)
        if (i<(*_data.pHarmonics) || (i>_avBoth.count()-1-(*_data.pHarmonics)))
            y[i]=x[i];
        else
        {
            y[i].x=0;y[i].y=0;
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
    fftc1dinv(y,_avBoth.count(),param);


    double _maxBackProj=0;
    double _minBackProj=10000;
    for (int i=_minFrame;i<_maxFrame;++i)
    {
        _maxBackProj = std::max(_maxBackProj,_avBoth.at(i)-abscomplex(x[i]));
        _minBackProj = std::min(_minBackProj,_avBoth.at(i)-abscomplex(x[i]));
        //_maxBackProj = std::max(_maxBackProj,abscomplex(x[i]));
        //_minBackProj = std::min(_minBackProj,abscomplex(x[i]));
    }


    QVector <float> _minusBackground;
    float val;
    for (long t=0;t<_data._frames;++t)
    {
        _data._both.pOrgData->append(_data._angles[t],_avBoth.at(t)/_maxValue);

        val = ((_avBoth.at(t)-abscomplex(x[t]))-_minBackProj)/(_maxBackProj-_minBackProj);
        //val = (abscomplex(x[t])-_minBackProj)/(_maxBackProj-_minBackProj);

        _minusBackground << val;
        _data._both.pLSeries->append(_data._angles[t],val);
    }

    QVector <int> _bPeaks;

    findMaxPositions(_minusBackground,_bPeaks,_minFrame,_maxFrame,(*_data.pLevelInPercent));

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
    _data._both.pIntervals = new QScatterSeries();
    _data._both.pIntervals->setName("intervals");
    _data._both.pIntervals->setPen(QPen(Qt::black));
    _data._both.pIntervals->setBrush(QBrush(Qt::blue));
    _data._both.pIntervals->setMarkerSize(5);
    for (int i=0;i<_intervals.count();++i)
    {
        _data._both.pIntervals->append(_intervals.at(i)[0],_minusBackground[_intervals.at(i)[3]]);
        _data._both.pIntervals->append(_intervals.at(i)[1],_minusBackground[_intervals.at(i)[4]]);
    }
    //_data._both = generateParam(_minusBackground,_minFrame,_maxFrame);

    _data._both.pPeaks= new QScatterSeries();
    _data._both.pPeaks->setName("peaks");
    _data._both.pPeaks->setPen(QPen(Qt::black));
    _data._both.pPeaks->setBrush(QBrush(Qt::red));
    _data._both.pPeaks->setMarkerSize(5);

    for (unsigned long i=0;i<_bPeaks.count();++i)
        _data._both.pPeaks->append(_data._angles.at(_bPeaks.at(i)),_minusBackground.at(_bPeaks.at(i)));

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

    if (pChart)
    {
        ui->pDataGV->scene()->removeItem(pChart);
    }

    _data._both.pCutOffLine=new QLineSeries();
    _data._both.pCutOffLine->setName("level");
    for (int i=0;i<37;++i)
        _data._both.pCutOffLine->append(i*10,_level);

    QLineSeries *pDataRegion = new QLineSeries();
    QLineSeries *pDataRegionLow = new QLineSeries();
    pDataRegion->setPen(QPen(Qt::gray));
    pDataRegionLow->setPen(QPen(Qt::gray));

    pDataRegion->append(_data._angles[_data._startFrame],0.0);
    pDataRegion->append(_data._angles[_data._startFrame],0.75);
    pDataRegionLow->append(_data._angles[_data._startFrame],0.0);
    pDataRegionLow->append(_data._angles[_data._startFrame],0.0);

    for (int i=_data._startFrame;i<_data._endFrame;i=i+5)
    {
        pDataRegion->append(_data._angles[i],0.75);
        pDataRegionLow->append(_data._angles[i],0.0);
    }
    pDataRegion->append(_data._angles[_data._endFrame],0.75);
    pDataRegion->append(_data._angles[_data._endFrame],0.0);
    pDataRegionLow->append(_data._angles[_data._endFrame],0.0);
    pDataRegionLow->append(_data._angles[_data._endFrame],0.0);

    QAreaSeries *pAreaSeries = new QAreaSeries(pDataRegion,pDataRegionLow);
    pAreaSeries->setBrush(QBrush(QColor(100,100,100,33)));
    pAreaSeries->setPen(QPen(Qt::gray));

    _data._both.pLSeries->setPen(QPen(Qt::black));
    QPen pmarker(Qt::white);
    pmarker.setWidthF(2);
    QPen pline(Qt::darkGray);
    pline.setStyle(Qt::DashDotLine);
    pline.setWidthF(2);
    int markerSize = 13;

    _data._both.pOrgData->setPen(QPen(Qt::darkGray));
    _data._both.pCutOffLine->setPen(pline);
    _data._both.pPeaks->setPen(pmarker);
    _data._both.pPeaks->setBrush(QBrush(Qt::black));
    _data._both.pPeaks->setMarkerSize(markerSize);
    _data._both.pIntervals->setPen(pmarker);
    _data._both.pIntervals->setBrush(QBrush(Qt::lightGray));
    _data._both.pIntervals->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
    _data._both.pIntervals->setMarkerSize(markerSize/2);

    //QPolarChart *pPolChart=new QPolarChart();
    QPolarChart *pPolChart=new QPolarChart();
    pPolChart->addSeries(_data._both.pOrgData);
    pPolChart->addSeries(pAreaSeries);
    _data._both.pOrgData->setName("raw data");
    pPolChart->addSeries(_data._both.pLSeries);
    _data._both.pLSeries->setName("filtered breathing events");
    pPolChart->addSeries(_data._both.pCutOffLine);
    pPolChart->addSeries(_data._both.pPeaks);
    pPolChart->addSeries(_data._both.pIntervals);
    pPolChart->legend()->setVisible(false);
    QValueAxis *angularAxis = new QValueAxis();
    angularAxis->setRange(0, 360);
    angularAxis->setTickCount(19);
    angularAxis->setTickInterval(20);
    QValueAxis *radialAxis = new QValueAxis();
    radialAxis->setLabelFormat("%.2f");
    radialAxis->setMax(1.1);
    radialAxis->setMin(0.0);
    pPolChart->addAxis(radialAxis, QPolarChart::PolarOrientationRadial);
    pPolChart->addAxis(angularAxis, QPolarChart::PolarOrientationAngular);
    _data._both.pLSeries->attachAxis(angularAxis);
    _data._both.pLSeries->attachAxis(radialAxis);
    _data._both.pOrgData->attachAxis(angularAxis);
    _data._both.pOrgData->attachAxis(radialAxis);
    _data._both.pCutOffLine->attachAxis(angularAxis);
    _data._both.pCutOffLine->attachAxis(radialAxis);
    _data._both.pPeaks->attachAxis(angularAxis);
    _data._both.pPeaks->attachAxis(radialAxis);
    pAreaSeries->attachAxis(angularAxis);
    pAreaSeries->attachAxis(radialAxis);
    _data._both.pIntervals->attachAxis(angularAxis);
    _data._both.pIntervals->attachAxis(radialAxis);
    pPolChart->setAnimationOptions(QChart::SeriesAnimations);
    pPolChart->legend()->setEnabled(false);

    pPolChart->setBackgroundBrush(QBrush(Qt::NoBrush));
    pPolChart->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding));
    pPolChart->setMinimumSize(ui->pDataGV->height(),ui->pDataGV->height());
    ui->pDataGV->scene()->setSceneRect(0,0,ui->pDataGV->width()-20,ui->pDataGV->height());
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
    txt+=QString("total time [s] = \t%1\n").arg(*_data.pTotalTime);
    if (_data._rotation) txt+=QString("rotation angle [°] = \t%1\n").arg(*_data.pRotationAngle);
    txt+=QString("body mass [g] = \t%1\n\n").arg(*_data.pBodyMass,0,'f',1);

    txt+=QString("# = \t%1\n").arg(_intervals.count());
    txt+=QString("br [s] = \t%1 ± %2\n").arg(_sum[0]*_timePerAngle,0,'f',3).arg(_quadSum[0]*_timePerAngle,0,'f',3);
    txt+=QString("du [s] = \t%1 ± %2\n").arg(_sum[3]*_timePerAngle,0,'f',3).arg(_quadSum[3]*_timePerAngle,0,'f',3);
    txt+=QString("in [s] = \t%1 ± %2\n").arg(_sum[1]*_timePerAngle,0,'f',3).arg(_quadSum[1]*_timePerAngle,0,'f',3);
    txt+=QString("iso = \t%1 ± %2\n").arg(_sum[2],0,'f',3).arg(_quadSum[2],0,'f',3);
    _data._heartRateInBPM!=-1 ?
    txt+=QString("\nhr [bpm] = %1\n").arg(_data._heartRateInBPM,0,'f',3) : txt+=QString("\nhr [bpm] = NaN\n");

    QFont f=font();
    f.setPixelSize(19);
    pResultTxtItem->setFont(f);
    pResultTxtItem->setPen(QPen(Qt::black));
    pResultTxtItem->setPos(ui->pDataGV->height()+90,80);
    pResultTxtItem->setText(txt);
    updateAndDisplayStatus();
}
