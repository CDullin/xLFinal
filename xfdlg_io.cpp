#include "xfdlg.h"
#include "ui_xfdlg.h"
#include "xfmsgdlg.h"
#include "tiffio.h"
#include <stdlib.h>

#include <QTextStream>
#include <QDateEdit>

using namespace std;

void xfDlg::exportResults()
{
    if (_data._dataValid && pResultTxtItem)
    {
        QString fname;
        fname += QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")+".csv";
        QFile f(fname);
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

            xfMSGDlg dlg(QString("resulted exported to:\n%1").arg(fname));
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
