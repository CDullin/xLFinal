#include "xfdlg.h"
#include "ui_xfdlg.h"
#include "xfmsgdlg.h"
#include "tiffio.h"
#include "xfimportdlg.h"
#include "xfprogressdlg.h"
#include <stdlib.h>
#include <math.h>

#include <QTextStream>
#include <QDateEdit>
#include <QFileInfo>
#include <stdlib.h>

using namespace std;

void xfDlg::saveSettings()
{
    QFile f("xLFinal_settings.dat");
    if (f.open(QFile::WriteOnly))
    {
        QTextStream t(&f);

        t << _data._fileName << endl;
        t << (*_data.pTotalTime) << endl;
        t << (*_data.pRotationAngle) << endl;
        t << (*_data.pBodyMass) << endl;
        t << (*_data.pLevelInPercent) << endl;
        t << (*_data.pHarmonics) << endl;
        t << (*_data.pTrendCorrTimeWindowInMS) << endl;

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
        _lastFileName = t.readLine();
        s=t.readLine();(*_data.pTotalTime)=s.toFloat();ui->pTotalTimeLCD->display(*_data.pTotalTime);
        s=t.readLine();(*_data.pRotationAngle)=s.toFloat();ui->pAngleLCD->display(*_data.pRotationAngle);
        s=t.readLine();(*_data.pBodyMass)=s.toFloat();ui->pMassLCD->display(*_data.pBodyMass);
        s=t.readLine();(*_data.pLevelInPercent)=s.toFloat();ui->pLevelLCD->display(*_data.pLevelInPercent);
        s=t.readLine();(*_data.pHarmonics)=s.toFloat();ui->pHarmonicsLCD->display(*_data.pHarmonics);
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


            if (ui->pTotalAngleTB->isChecked())
            {
                QStringList lst=pResultTxtItem->text().split("\n");
                for (QStringList::iterator it=lst.begin();it!=lst.end();++it)
                {
                    QString s=*it;
                    s.remove(QChar('\t'));
                    s.replace(" = ",",");
                    s.replace(" ± ",",");
                    t << s << endl;
                }
            }
            else
            {
                QStringList lst=pResultTxtItem->text().split("\n");
                QStringList _append;
                _append.append(lst.at(0));
                _append.append(QString("region,#,breathing rate {s},std_breathing rate,insp. time {s},"
                                       "std_insp. time,isotrophy index,std_isotrophy index,anisotrophy index,"
                                       "std_anisotrophy index,tidal flow,std_tidal flow,"
                                       "baseline,std_baseline,decay rate {Hz},R^2,"
                                       "heart rate {bpm},std_heart rate"));
                for (int i=3;i<lst.count()-1;i=i+2)
                    _append.append((lst.at(i)+lst.at(i+1)));
                for (QStringList::iterator it=_append.begin();it!=_append.end();++it)
                {
                    QString s=*it;
                    s.replace(QChar('\t'),QChar(','));
                    s.replace('\n',",");
                    s.replace("  ",",");
                    s.replace(" [",",");
                    s.remove("]");
                    s.replace(" ± ",",");
                    t << s << endl;
                }
            }

            f.close();

            xfMSGDlg dlg("..."+fname.right(50));
            dlg.exec();
        }
        else
            emit MSG(QString("Could not open %1 for writing.").arg(fname),true);

        // export sorted data
        if (_data._rotation) saveMultiFrameTIFF();
    }
}

void myImageCleanupHandler(void *info)
{
    _TIFFfree(info);
}

void xfDlg::saveMultiFrameTIFF()
{
    // continue here
    QFileInfo info(_data._fileName);
    QString fname = info.absolutePath()+"/"+info.baseName()+"_"+QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")+"_exp.tif";
    const char* _fileName = fname.toUtf8().constData();
    TIFF *tif = TIFFOpen(_fileName,"w");
    float _level = (*_data.pLevelInPercent)/100.0f;

    QImage img=readTIFFrame(0);

    uint32 imageWidth   = img.width();
    uint32 imageLength  = img.height();
    uint32 bitsPerPixel = 16;
    unsigned short resUnit;
    uint32 row;
    uint64 offset;
    tdata_t buf;
    float pixInX;

    quint16* pBuffer=(uint16*)_TIFFmalloc(imageWidth*imageLength*sizeof(uint16));

    _fileName = _data._fileName.toUtf8().constData();
    TIFF *tifLoad = TIFFOpen(_fileName,"r");

    xfProgressDlg dlg(0);
    dlg.setRange(0,_data._frames-1);
    dlg.setValue(0);
    //connect(&dlg,SIGNAL(aborted()),this,SLOT(abortCurrentFunction()));
    dlg.show();

    QFile f(info.absolutePath()+"/"+info.baseName()+"_"+QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")+"_Angle.prm");
    f.open(QFile::WriteOnly);
    QTextStream t(&f);

    if (tif && tifLoad)
    {
        long j=0;
        for (long i=0;i<_data._frames;++i)
        {
            dlg.setValue(i);
            if (_data._both.pLSeries->at(i).y()<_level)
            {
                // save angle
                t << (*_data.pRotationAngle)/(float)_data._frames*(float)i/180.0f*M_PI << endl;

                TIFFSetField(tif, TIFFTAG_PAGENUMBER, j, j);
                TIFFSetField(tif, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
                TIFFSetField(tif, TIFFTAG_IMAGEWIDTH,imageWidth);
                TIFFSetField(tif, TIFFTAG_IMAGELENGTH,imageLength);
                TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,bitsPerPixel);
                TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL,1);
                TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
                TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
                TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, (unsigned int) - 1));

                // read image
                TIFFSetDirectory(tifLoad,i);
                offset = (uint64)pBuffer;
                buf = _TIFFmalloc(TIFFScanlineSize(tifLoad));
                for (row = 0; row < imageLength; row++)
                {
                    TIFFReadScanline(tifLoad, buf, row);
                    memcpy((void*)offset,buf,imageWidth*2);
                    offset+=imageWidth*2;
                }
                _TIFFfree(buf);

                offset = (uint64)pBuffer;
                for (row = 0; row < imageLength; row++)
                {
                    TIFFWriteScanline(tif, (void*)offset, row, 0);
                    offset+=imageWidth*2;
                }

                TIFFWriteDirectory(tif);
                j++;

            }
        }

        TIFFClose(tif);
        TIFFClose(tifLoad);
        f.close();
    }
    dlg.hide();
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

        quint16* pBuffer=(quint16*)_TIFFmalloc(imageWidth*imageLength*sizeof(quint16));
        quint8* pU8Buffer=(quint8*)_TIFFmalloc(imageWidth*imageLength);
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
            pU8Buffer[i]=(float)(pBuffer[i]-_minVal)*255.0f/(float)(_maxVal-_minVal);
        _TIFFfree(pBuffer);

        // cleanup function
        img = QImage((uchar*)pU8Buffer,imageWidth,imageLength,imageWidth,QImage::Format_Grayscale8,myImageCleanupHandler);

        TIFFClose(tif);
    }

    return img;
}

void xfDlg::import()
{
    xfImportDlg dlg(0);
    if (!_lastFileName.isEmpty()) _data._fileName = _lastFileName;
    if (!_data._fileName.isEmpty())
    {
        QFileInfo info(_data._fileName);
        dlg.setCurrentFile(info.absolutePath());
    }
    connect(&dlg,SIGNAL(MSG(const QString&,const bool&)),this,SLOT(MSGSlot(const QString&,const bool&)));
    connect(&dlg,SIGNAL(selectedTIFFile(const QString&)),this,SLOT(selectedImportFile(const QString&)));
    connect(&dlg,SIGNAL(selectedCSVFile(const QString&)),this,SLOT(selectedCSVFile(const QString&)));
    if (dlg.exec()==QDialog::Accepted)
    {
        _lastFileName=_data._fileName;
        xfMSGDlg dlg("Please set correct aquistion time and rotation angle!");
        dlg.exec();
    }
}

void xfDlg::selectedImportFile(const QString& fname)
{
    emit MSG(QString("%1 file selected").arg(fname));

    // analyse data
    QFileInfo info(fname);
    ui->pFileNameLEdit->setText(info.fileName());
    _data._fileName = fname;

    // check how many frames
    _data._frames=0;
    const char* _fileName = info.absoluteFilePath().toLatin1().constData();
    TIFF *tif = TIFFOpen(_fileName,"r");
    if (tif)
    {
        do
        {
          _data._frames++;
        } while (TIFFReadDirectory(tif));
        TIFFClose(tif);
    }

    if (QFileInfo(info.absolutePath()+"/angle.prm").exists() || QFileInfo(info.absolutePath()+"/Angle.prm").exists())
    {
        _data._rotation = true;
        _data._startFrame = (float)_data._frames / 360.0 * 45.0;
        _data._endFrame = (float)_data._frames / 360.0 * 315.0;
    }
    else
    {
        _data._rotation = false;
        *_data.pTotalTime = 34.0;
        ui->pTotalTimeLCD->display(*_data.pTotalTime);
        _data._startFrame=0;
        _data._endFrame=_data._frames-1;

    }

    ui->pFrameDial->setRange(0,_data._frames-1);

    if (_data._frames<2)
    {
        emit MSG("Insufficient frames found.",true);
        _data._dataValid = false;
    }
    else
    {
        _data._dataValid = true;
        ui->pFrameDial->setValue(0);
    }

    p3DFrameItem = nullptr;
    pCornerPixmapItem = nullptr;

    dispFrame();
    createStandardPathItems();
    updateAndDisplayStatus();

    if (_data._rotation && !ui->pTotalAngleTB->isChecked()) ui->pTotalAngleTB->animateClick();
    if (!_data._rotation && ui->pTotalAngleTB->isChecked()) ui->pTotalAngleTB->animateClick();
}
