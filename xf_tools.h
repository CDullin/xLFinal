#ifndef XF_TOOLS_H
#define XF_TOOLS_H

#include <QVector>
#include "xf_types.h"

void getIntervals(QVector<float> values, QVector<int> &peaks, QVector<int> &interval, float fps,float movFilterWidth);
QVector <float> movAvFilter(QVector <float> values, float halfFrameWidth);
QVector <float> adaptiveMovAvFilter(QVector <float> values, float halfFrameWidth);
void findMaxPositions(QVector<float> values, QVector<int> &_peaks,int minFrame,int maxFrame, float lvl);
void getMeanAndStd(QVector<int> pos, float &mean, float &std, const int min=-1, const int max=-1);
void getMeanAndStd(QVector<float> v, float &mean, float &std, const int min=-1, const int max=-1);
XLFParam generateParam(QVector <float> values,int minFrame,int maxFrame,float fps,float lvl, float _intervalFilter);

#endif // XF_TOOLS_H
