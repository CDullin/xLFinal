#include "xf_tools.h"
#include <stdlib.h>
#include <math.h>
#include <QMessageBox>
#include <QObject>
#include "../../3rd_party/Alglib/cpp/src/stdafx.h"
#include "../../3rd_party/Alglib/cpp/src/interpolation.h"

using namespace alglib;
using namespace std;

void function_cx_1_func(const real_1d_array &c, const real_1d_array &x, double &func, void *ptr)
{
    // this callback calculates f(c,x)=exp(-c0*sqr(x0))
    // where x is a position on X-axis and c is adjustable parameter
    func = c[2]*exp(-c[1]*pow(x[0],2))+c[0];
}

void nonLinearFit(QVector <float> xvalues,QVector <float> yvalues,QVector <float>& param)
{
    try
    {
        //
        // In this example we demonstrate exponential fitting
        // by f(x) = c2*exp(-c1*x^2)+c0
        // using function value only.
        //
        // Gradient is estimated using combination of numerical differences
        // and secant updates. diffstep variable stores differentiation step
        // (we have to tell algorithm what step to use).
        //

        // fill in values ....>>>>
        // real_2d_array x = "[[-1],[-0.8],[-0.6],[-0.4],[-0.2],[0],[0.2],[0.4],[0.6],[0.8],[1.0]]";
        // real_1d_array y = "[0.223130, 0.382893, 0.582748, 0.786628, 0.941765, 1.000000, 0.941765, 0.786628, 0.582748, 0.382893, 0.223130]";

        /*
        double *buffer = (double*)malloc(xvalues.count()*sizeof(double));
        double *ybuffer = (double*)malloc(xvalues.count()*sizeof(double));
        for (int i=0;i<xvalues.count();++i)
        {
            buffer[i]=xvalues.at(i);
            ybuffer[i]=yvalues.at(i);
        }
        real_2d_array x;x.setlength(xvalues.count(),1);
        real_1d_array y;y.setlength(xvalues.count());
        x.setcontent(xvalues.count(),1,buffer);
        y.setcontent(xvalues.count(),ybuffer);
        */

        real_1d_array y;
        y.setlength(yvalues.count());
        for (int i=0;i<yvalues.count();++i)
            y[i]=yvalues.at(i);

        real_2d_array x;
        x.setlength(xvalues.count(),1);
        for (int i=0;i<xvalues.count();++i)
            x[i][0]=xvalues.at(i);

        // x(2)./exp(-x(1)*xdata^2)+x(0)

        // x(2)=0.039
        // x(1)=45.85 [tau]
        // x(0)=0.68

        real_1d_array c = "[0.039, 50.0, 0.68]";
        double epsx = 0.00000001;
        ae_int_t maxits = 0;
        ae_int_t info;
        lsfitstate state;
        lsfitreport rep;
        double diffstep = 0.000001;

        //
        // Fitting without weights
        //
        lsfitcreatef(x, y, c, diffstep, state);
        lsfitsetcond(state, epsx, maxits);
        alglib::lsfitfit(state, function_cx_1_func);
        lsfitresults(state, info, c, rep);

        if (info!=-8)
        {
            param.append(c.getcontent()[1]);
            param.append(rep.r2);
        }
        else
        {
            param.append(-1);
            param.append(-1);
            //emit MSG("erorr in breathing event fit");
        }
    }
    catch (...)
    {
        param.append(-1);
        param.append(-1);
        //emit MSG("erorr in breathing event fit");
    }
    // error?

}


QVector <float> movAvFilter(QVector <float> values, float halfFrameWidth)
{
    QVector <float> res;
    float sumBuffer,count;
    for (int i=0;i<values.count();++i)
    {
        sumBuffer=0.0f;count=0.0f;
        for (int j=std::max(0.0f,(float)i-halfFrameWidth);j<std::min((float)values.count(),(float)i+halfFrameWidth);++j)
        {
            sumBuffer+=values.at(j);
            count++;
        }
        count>0 ? res.append(sumBuffer/count) : res.append(0);
    }
    return res;
}

QVector <float> adaptiveMovAvFilter(QVector <float> values, float halfFrameWidth)
{
    QVector <float> _filtered = movAvFilter(values,halfFrameWidth);
    float _weight;

    QVector <float> res;
    float sumBuffer,count;
    for (int i=0;i<values.count();++i)
    {
        sumBuffer=0.0f;count=0.0f;
        for (int j=std::max(0.0f,(float)i-halfFrameWidth);j<std::min((float)values.count(),(float)i+halfFrameWidth);++j)
        {
            _weight = 1.0f / (fabs(values.at(j)-_filtered.at(j))+0.1f);
            sumBuffer+=_weight*values.at(j);
            count+=_weight;
        }
        count>0 ? res.append(sumBuffer/count) : res.append(0);
    }
    return res;
}


void findMaxPositions(QVector<float> values, QVector<int> &_peaks,int minFrame,int maxFrame, float lvl)
{
    if (values.length()>2)
    {
        float _minVal,_maxVal;
        _minVal=65535.0;
        _maxVal=-_minVal;
        for (int i=minFrame;i<maxFrame;++i)
        {
            _minVal=std::min(_minVal,values.at(i));
            _maxVal=std::max(_maxVal,values.at(i));
        }

        float _lvlFac = lvl/100.0f;
        float _lvl = _minVal+(_maxVal-_minVal)*_lvlFac;
        bool _startPntFound=false;
        QVector <int> _region;
        for (int i=minFrame+1;i<maxFrame-1;++i)
        {
            if (values.at(i)>_lvl && values.at(i-1)<_lvl)
            {
                _startPntFound=true;
                _region.clear();
            }
            if (_startPntFound) _region.append(i);
            if (values.at(i)>_lvl && values.at(i+1)<_lvl && _startPntFound)
            {
                // endpoint found
                float _maxBrightnessInRegion=0.0f;
                int _maxBrightnessPos=-1;
                for (int j=0;j<_region.length();++j)
                    if (values.at(_region.at(j))>_maxBrightnessInRegion)
                    {
                        _maxBrightnessPos=_region.at(j);
                        _maxBrightnessInRegion=values.at(_region.at(j));
                    }
                if (_maxBrightnessPos!=-1)
                    _peaks.append(_maxBrightnessPos);
                _startPntFound=false;
            }
        }
    }
}

void getMeanAndStd(QVector<int> pos, float &mean, float &std, const int min, const int max)
{
    int sp = 0;
    int ep = pos.count();
    if (min!=-1) sp=min;
    if (max!=-1) ep=max;

    float sumBuffer=0.0f;
    float quadSumBuffer=0.0f;
    for (int i=sp;i<ep-1;++i)
    {
        sumBuffer+=pos.at(i+1)-pos.at(i);
        quadSumBuffer+=pow(pos.at(i+1)-pos.at(i),2.0);
    }
    mean = sumBuffer / (float)(ep-sp-1);
    std = quadSumBuffer / (float)(ep-sp-1) - pow(mean,2.0);
}

void getMeanAndStd(QVector<float> v, float &mean, float &std, const int min, const int max)
{
    int sp = 0;
    int ep = v.count();
    if (min!=-1) sp=min;
    if (max!=-1) ep=max;

    float sumBuffer=0.0f;
    float quadSumBuffer=0.0f;
    for (int i=sp;i<ep;++i)
    {
        sumBuffer+=v.at(i);
        quadSumBuffer+=pow(v.at(i),2.0);
    }
    mean = sumBuffer / (float)(ep-sp);
    std = quadSumBuffer / (float)(ep-sp) - pow(mean,2.0);
}

void getIntervals(QVector<float> values, QVector<int> &peaks, QVector<int> &interval, float fps,float movFilterWidth)
{
//    float fps = (float)_data._frames/(*_data.pTotalTime);
//    float movFilterWidth = (*_data.pTrendCorrTimeWindowInMS)/1000.0f*fps;
//    QVector <float> filtered = movAvFilter(values,movFilterWidth);
    QVector <float> filtered  = values;
    for (int p=1;p<peaks.count();++p)       // we cut 1st end last peak
    {
        float val = filtered.at(peaks.at(p));
        bool found=false;
        for (int j=peaks.at(p);j>peaks.at(p-1) && !found;--j)
        {
            if (val>=filtered.at(j))
            {
                val = filtered.at(j);
            }
            else
            {
                found = true;
                interval.append(j+1);
            }
        }
    }
}

XLFParam generateParam(QVector <float> values,int minFrame,int maxFrame,float fps,float lvl, float _intervalFilter)
{
/*
    float fps = (float)(_data._frames)/(*_data.pTotalTime);
    float lvl = (*_data.pLevelInPercent);
    int _width = (*_data.pMinIntervalLengthInMS)/1000.f*fps;
*/
    XLFParam param;
    param._valid = true;
    findMaxPositions(values,param._peaks,minFrame,maxFrame,lvl);

    if (param._peaks.count()<3)
    {
        QMessageBox::critical(0,"Critical Error",QString("Less than 3 breathing found in interval [%1 .. %2]. Calculation aborted.").arg(minFrame).arg(maxFrame));
        //emit MSG(QString("Less than 3 breathing events found in interval [%1 .. %2]. Calculation aborted.").arg(minFrame).arg(maxFrame));
        param._valid = false;
        return param;
    }

    getIntervals(values,param._peaks,param._intervals,fps,_intervalFilter);
    param._peaks.removeLast();param._peaks.removeFirst();
    getMeanAndStd(values,param._avL,param._stdL,minFrame,maxFrame);

    float _minVal = values.at(minFrame);
    float _maxVal = values.at(minFrame);
    for (int i=minFrame;i<maxFrame;++i)
    {
        _minVal = std::min(_minVal,values.at(i));
        _maxVal = std::max(_maxVal,values.at(i));
    }

    param._cutOff = _minVal+(_maxVal-_minVal)*lvl/100.0f;

    for (int i=0;i<param._peaks.count();++i)
    {
        bool _found=false;
        if (i<param._intervals.count()-1)
        {
            for (int j=param._peaks.at(i);j<param._intervals.at(i+1) && !_found;++j)
            {
                if (values.at(j) < param._cutOff)
                {
                    _found = true;
                    param._D.append(j);
                }
            }
        }
    }

    float sumBuffer=0.0f;
    float quadSumBuffer=0.0f;
    float count=0.0f;
    for (int i=0;i<param._intervals.count()-1;++i)
    {
        float iBuffer=0.0f;
        if (i<param._D.count())
        {
            for (int j=param._intervals.at(i)+1;j<param._D.at(i);++j)
            {
                iBuffer+=values.at(j)-values.at(param._intervals.at(i));
            }
            sumBuffer+=iBuffer;
            quadSumBuffer+=pow(iBuffer,2.0);
            count++;
        }
    }
    count>0 ? param._TV =sumBuffer/count : param._TV=0;
    count>0 ? param._stdTV = quadSumBuffer/count - pow(param._TV,2.0) : param._stdTV = 0.0f;

    param._TV/=fps;
    param._stdTV/=fps;

    sumBuffer=0.0f;
    quadSumBuffer=0.0f;
    count=0.0f;
    for (int i=0;i<param._intervals.count()-1;++i)
    {
        float iBuffer=0.0f;
        float iCount=0.0f;
        if (i<param._D.count())
        {
            for (int j=param._D.at(i);j<param._intervals.at(i+1);++j)
            {
                iBuffer+=values.at(j);
                iCount++;
            }
            iCount>0 ? sumBuffer+=iBuffer/iCount : sumBuffer+=0.0f;
            quadSumBuffer+=pow(iBuffer/iCount,2.0);
            count++;
        }
    }
    count>0 ? param._ATrp =sumBuffer/count : param._ATrp = 0.0f;
    count>0 ? param._stdATrp = quadSumBuffer/count - pow(param._ATrp,2.0) : param._stdATrp = 0.0f;

    param._ATrp/=fps;
    param._stdATrp/=fps;

    getMeanAndStd(param._peaks,param._avL,param._stdL);
    param._avL/=fps;
    param._stdL/=fps;

    sumBuffer=0.0f;
    quadSumBuffer=0.0f;
    count=0.0f;
    for (int i=0;i<param._peaks.count()-1;++i)
    {
        if (i<param._intervals.count())
        {
            sumBuffer+=param._peaks.at(i)-param._intervals.at(i);
            quadSumBuffer+=pow(param._peaks.at(i)-param._intervals.at(i),2.0);
            count++;
        }
    }
    count>0 ? param._tin =sumBuffer/count : param._tin = 0.0f;
    count>0 ? param._stdtin = quadSumBuffer/count - pow(param._tin,2.0) : param._stdtin = 0.0f;
    param._tin/=fps;
    param._stdtin/=fps;

    param._Iso = param._tin / param._avL;
    param._stdIso = 1.0/param._avL*param._stdL+fabs(param._tin/pow(param._avL,2.0))*param._stdL;

    float sumBufferA=0.0f;
    float quadSumBufferA=0.0f;
    count=0.0f;
    float sumBufferB=0.0f;
    float quadSumBufferB=0.0f;
    for (int i=0;i<param._peaks.count()-1;++i)
    {
        if (i<param._intervals.count() && i<param._D.count())
        {
            sumBufferA+=param._D.at(i)-param._intervals.at(i);
            quadSumBufferA+=pow(param._D.at(i)-param._intervals.at(i),2.0);
            sumBufferB+=param._intervals.at(i+1)-param._D.at(i);
            quadSumBufferB+=pow(param._intervals.at(i+1)-param._D.at(i),2.0);
            count++;
        }
    }
    count>0 ? param._AnIso =sumBufferA/sumBufferB : param._AnIso = 0.0f;
    count>0 ? param._stdAnIso = quadSumBufferA/quadSumBufferB - pow(param._AnIso,2.0) : param._stdAnIso = 0.0f;


    // x(1)./exp(xdata.*x(2))+x(3)
    // x(1)=0.0671, x(2)=11.1576 [tau], x(3)=0.6653

    QVector <float> fvalues;
    QVector <float> xvalues;
    QVector <float> fcount;

    for (int i=0;i<param._intervals.count()-1;++i)
    {
        if (i<param._peaks.count())
        {
            for (int j=param._peaks.at(i);j<param._intervals.at(i+1);++j)
            //for (int j=param._peaks.at(i);j<param._D.at(i+1);++j)
            {
                fvalues.append(values.at(j));
                xvalues.append((float)(j-param._peaks.at(i))/fps);
            }
        }
    }
    QVector <float> parameter;
    nonLinearFit(xvalues,fvalues,parameter);
    param._decayRate = parameter.at(0);
    param._RSquaredDecayRate = parameter.at(1); // R2

    // heart beat
    // FFT left lobe

    return param;
}
