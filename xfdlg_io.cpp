#include "xfdlg.h"
#include "ui_xfdlg.h"
#include "xfmsgdlg.h"
#include "tiffio.h"
#include <stdlib.h>

#include <QTextStream>
#include <QDateEdit>
#include <QFileInfo>

using namespace std;

void xfDlg::saveSettings()
{
    QFile f("xLFinal_settings.dat");
    if (f.open(QFile::WriteOnly))
    {
        QTextStream t(&f);

        t << (*_data.pTotalTime) << Qt::endl;
        t << (*_data.pRotationAngle) << Qt::endl;
        t << (*_data.pBodyMass) << Qt::endl;
        t << (*_data.pLevelInPercent) << Qt::endl;
        t << (*_data.pHarmonics) << Qt::endl;
        t << (*_data.pPeakCorrTimeWindowInMS) << Qt::endl;
        t << (*_data.pTrendCorrTimeWindowInMS) << Qt::endl;

        f.close();
    }
}

void xfDlg::restoreSettings()
{
    QFile f("xLFinal_settings.dat");
    if (f.open(QFile::ReadOnly))
    {
        QTextStream t(&f);

        QString s;
        s=t.readLine();(*_data.pTotalTime)=s.toFloat();ui->pTotalTimeLCD->display(*_data.pTotalTime);
        s=t.readLine();(*_data.pRotationAngle)=s.toFloat();ui->pAngleLCD->display(*_data.pRotationAngle);
        s=t.readLine();(*_data.pBodyMass)=s.toFloat();ui->pMassLCD->display(*_data.pBodyMass);
        s=t.readLine();(*_data.pLevelInPercent)=s.toFloat();ui->pLevelLCD->display(*_data.pLevelInPercent);
        s=t.readLine();(*_data.pHarmonics)=s.toFloat();ui->pHarmonicsLCD->display(*_data.pHarmonics);
        s=t.readLine();(*_data.pPeakCorrTimeWindowInMS)=s.toFloat();ui->pPeakCorrTimeWindowLCD->display(*_data.pPeakCorrTimeWindowInMS);
        s=t.readLine();(*_data.pTrendCorrTimeWindowInMS)=s.toFloat();ui->pTrendCorrTimeWindowLCD->display(*_data.pTrendCorrTimeWindowInMS);

        f.close();
    }
}

void xfDlg::defaultSettings()
{
    *_data.pTotalTime=34;ui->pTotalTimeLCD->display(*_data.pTotalTime);
    *_data.pRotationAngle=360;ui->pAngleLCD->display(*_data.pRotationAngle);
    *_data.pBodyMass=18;ui->pMassLCD->display(*_data.pBodyMass);
    *_data.pLevelInPercent=40;ui->pLevelLCD->display(*_data.pLevelInPercent);
    *_data.pHarmonics=10;ui->pHarmonicsLCD->display(*_data.pHarmonics);
    *_data.pPeakCorrTimeWindowInMS=50;ui->pPeakCorrTimeWindowLCD->display(*_data.pPeakCorrTimeWindowInMS);
    *_data.pTrendCorrTimeWindowInMS=1500;ui->pTrendCorrTimeWindowLCD->display(*_data.pTrendCorrTimeWindowInMS);
}

void xfDlg::exportResults()
{
    if (_data._dataValid && pResultTxtItem)
    {
        QFileInfo info(_data._fileName);

        QString fname;        
        fname += info.absolutePath()+"/"+info.baseName()+"_"+QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
                
        // export graph
        QPixmap pix = ui->pDataGV->grab(ui->pDataGV->mapFromScene(pChart->mapToScene(pChart->boundingRect())).boundingRect());
        pix.save(fname+".png","PNG");
        emit MSG(fname+".png has been saved");
        
        
        QFile f(fname+".csv");
        if (f.open(QFile::WriteOnly))
        {
            QTextStream t(&f);

            QStringList lst=pResultTxtItem->text().split("\n");
            for (QStringList::iterator it=lst.begin();it!=lst.end();++it)
            {
                QString s=*it;
                s.remove(QChar('\t'));
                s.replace(" = ",",");
                t << s << Qt::endl;
            }

            f.close();

            xfMSGDlg dlg("..."+fname.right(50));
            dlg.exec();
        }
        else
            emit MSG(QString("Could not open %1 for writing.").arg(fname),true);
    }
}

void myImageCleanupHandler(void *info)
{
    _TIFFfree(info);
}

QImage xfDlg::readTIFFrame(int frameNr)
{
    QImage img;

    const char* _fileName = _data._fileName.toUtf8().constData();
    TIFF *tif = TIFFOpen(_fileName,"r");
    if (tif)
    {
        TIFFSetDirectory(tif,frameNr);

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
        offset = (uint64)pBuffer;
        buf = _TIFFmalloc(TIFFScanlineSize(tif));
        for (row = 0; row < imageLength; row++)
        {
            TIFFReadScanline(tif, buf, row);
            memcpy((void*)offset,buf,imageWidth*2);
            offset+=imageWidth*2;
        }
        _TIFFfree(buf);

        quint16 _minVal = pBuffer[0];
        quint16 _maxVal = pBuffer[0];
        long size = imageWidth*imageLength;
        for (int i=0;i<size;++i)
        {
            _minVal = min(_minVal,pBuffer[i]);
            _maxVal = max(_maxVal,pBuffer[i]);
        }

        for (int i=0;i<size;++i)
            pBuffer[i]=(float)(pBuffer[i]-_minVal)*65535.0f/(float)(_maxVal-_minVal);

        // cleanup function
        img = QImage((uchar*)pBuffer,imageWidth,imageLength,imageLength*2,QImage::Format_Grayscale16,myImageCleanupHandler);

        TIFFClose(tif);
    }

    return img;
}
