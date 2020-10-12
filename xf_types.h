#ifndef XF_TYPES_H
#define XF_TYPES_H

#include <QVector>
#include <QString>

#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QChart>

using namespace QtCharts;

class XLFParam
{
public:
    float _valid;                   // an error has occured during calculation
    float _count;                   // counts of breathing events minus first and last one
    float _avL,_stdL;               // breathing length and std
    float _tin,_stdtin;             // inspiration time
    float _Iso,_stdIso;             // tin/L
    float _AnIso,_stdAnIso;         // D-end / start-D
    float _TV,_stdTV;               // AuC breathing event
    float _ATrp,_stdATrp;           // average level in expiration
    float _decayRate,_RSquaredDecayRate; // decay rate in expiration 1/x fit
    float _inspRate,_stdInspRate;   // slope of jump from min to peak
    float _cutOff;                  // level function
    // only for display
    QVector <int> _peaks,_intervals,_D;
    // Series for display
    QLineSeries     *pLSeries       = nullptr;
    QLineSeries     *pOrgData       = nullptr;
    QScatterSeries  *pIntervals     = nullptr;
    QScatterSeries  *pPeaks         = nullptr;
    QScatterSeries  *pDSeries       = nullptr;
    QLineSeries     *pCutOffLine    = nullptr;

    XLFParam(){};
    void setVisible(bool b)
    {
        pLSeries->setVisible(b);
        pIntervals->setVisible(b);
        pPeaks->setVisible(b);
        pDSeries->setVisible(b);
        pCutOffLine->setVisible(b);
    };

    void addToChart(QChart* pChart)
    {
        pChart->addSeries(pCutOffLine);
        pChart->addSeries(pLSeries);
        pChart->addSeries(pPeaks);
        pChart->addSeries(pIntervals);
        pChart->addSeries(pDSeries);
    }
};

struct DataContainer
{
public:
    float _heartRateInBPM = -1.0f;
    float *pBodyMass = nullptr;
    float *pTotalTime = nullptr;
    float *pRotationAngle = nullptr;
    bool _rotation = false;
    long _frames = -1;
    bool _dataValid = false;
    QString _fileName = "";
    QVector <float> _angles;
    XLFParam _left,_right,_both;
    long _startFrame=-2;
    long _endFrame=-1;

    float *pHarmonics = nullptr;
    float *pTrendCorrTimeWindowInMS = nullptr;
    float *pLevelInPercent = nullptr;

    DataContainer()
    {
        pBodyMass = new float(18);
        pTotalTime = new float(17);
        pRotationAngle = new float(360);
        pHarmonics = new float(10);
        pTrendCorrTimeWindowInMS = new float(1000);
        pLevelInPercent = new float(40);
    };
};


#endif // XF_TYPES_H
