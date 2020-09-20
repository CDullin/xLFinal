#include "xfdlg.h"
#include "ui_xfdlg.h"
#include "xfimportdlg.h"
#include "xfprogressdlg.h"
#include "xfinsertnrdlg.h"
#include "tiffio.h"
#include <stdlib.h>
#include <QtCharts>
#include "../../3rd_party/Alglib/cpp/src/stdafx.h"
#include "../../3rd_party/Alglib/cpp/src/interpolation.h"
#include "../../3rd_party/Alglib/cpp/src/fasttransforms.h"
#include "xfmsgdlg.h"
#include "xf_tools.h"
#include "xf_types.h"
#include "lcdconnector.h"

using namespace alglib;
using namespace QtCharts;
using namespace std;

#include <QTimer>
#include <QDateTime>
#include <QBitmap>
#include <QFileInfo>

xfDlg::xfDlg(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::xfDlg)
{
    ui->setupUi(this);
    setWindowFlag(Qt::FramelessWindowHint,true);

    ui->pGV->setScene(new QGraphicsScene());
    ui->pDataGV->setScene(new QGraphicsScene());

    QTimer *pTimer = new QTimer(this);
    pTimer->setInterval(1000);
    connect(pTimer,SIGNAL(timeout()),SLOT(updateClock()));
    pTimer->start();

    pPlayTimer = new QTimer(this);
    connect(pPlayTimer,SIGNAL(timeout()),this,SLOT(dispNextFrame()));

    connect(ui->pExitButton,SIGNAL(clicked()),this,SLOT(close()));
    connect(ui->pInputTB,SIGNAL(clicked()),this,SLOT(import()));
    connect(ui->pFrameDial,SIGNAL(valueChanged(int)),this,SLOT(displayFrame(int)));
    connect(ui->pPlayTB,SIGNAL(toggled(bool)),this,SLOT(startStopPlayTime(bool)));
    connect(ui->pTotalAngleTB,SIGNAL(toggled(bool)),this,SLOT(rotationModeOnOff(bool)));
    connect(ui->pStartTB,SIGNAL(clicked()),this,SLOT(start()));
    connect(ui->pExportTB,SIGNAL(clicked()),this,SLOT(exportResults()));
    connect(ui->pVisCB,SIGNAL(activated(int)),this,SLOT(displaySeries(int)));
    connect(ui->pHideShowControlPointsTB,SIGNAL(toggled(bool)),SLOT(hideShowControlPoints(bool)));
    connect(ui->pMirrorXTB,SIGNAL(toggled(bool)),this,SLOT(dispFrame()));
    connect(ui->pMirrorYTB,SIGNAL(toggled(bool)),this,SLOT(dispFrame()));

    ui->pTabWdgt->setCurrentIndex(0);

    new LCDConnector(ui->pTotalTimeLCD,ui->pUpTotalTimeTB,ui->pDownTotalTimeTB,5,20000,1,_data.pTotalTime);
    new LCDConnector(ui->pAngleLCD,ui->pUpAngleTB,ui->pDownAngleTB,180,20000,1,_data.pRotationAngle);
    new LCDConnector(ui->pMassLCD,ui->pUpMassTB,ui->pDownMassTB,10,30,0.1,_data.pBodyMass);
    new LCDConnector(ui->pLevelLCD,ui->pUpLevel,ui->pDownLevel,5,95,1,_data.pLevelInPercent);
    new LCDConnector(ui->pHarmonicsLCD,ui->pUpHarmonics,ui->pDownHarmonics,2,30,1,_data.pHarmonics);
    new LCDConnector(ui->pTrendCorrTimeWindowLCD,ui->pUpTrend,ui->pDownTrend,0,5000,100,_data.pTrendCorrTimeWindowInMS);
    new LCDConnector(ui->pPeakCorrTimeWindowLCD,ui->pUpPeak,ui->pDownPeak,0,500,10,_data.pPeakCorrTimeWindowInMS);
    new LCDConnector(ui->pMinIntervalLengthLCD,ui->pUpInterval,ui->pDownInterval,0,500,10,_data.pMinIntervalLengthInMS);

    QPixmap up(":/images/up.png");
    up = up.scaledToWidth(30);
    QPixmap empty(40,40);
    empty.fill(QColor(0,0,0,0));
    QPainter pain(&empty);
    pain.drawPixmap(5,5,up);
    pain.end();
    QBitmap upbmp = empty.createHeuristicMask();

    QPixmap down(":/images/down.png");
    up = up.scaledToWidth(30);
    empty.fill(QColor(0,0,0,0));
    QPainter pain2(&empty);
    pain.drawPixmap(5,5,down);
    pain.end();
    QBitmap downbmp = empty.createHeuristicMask();

    ui->pStartTimeTB->setMask(upbmp);
    ui->pEndTimeTB->setMask(downbmp);
    connect(ui->pStartTimeTB,SIGNAL(pressed()),this,SLOT(resetStartFrame()));
    connect(ui->pEndTimeTB,SIGNAL(pressed()),this,SLOT(resetEndFrame()));
    resetEndFrame();
    resetStartFrame();
    connect(this,SIGNAL(MSG(const QString&,const bool&)),this,SLOT(MSGSlot(const QString&,const bool&)));
    installEventFilter(this);
    ui->pFrameDial->installEventFilter(this);
    selectedImportFile("/run/media/heimdall/ex_puppy/rotation-lung-function/CM_20200828_095023/CM_20200828_095023.TIF");
}

void xfDlg::resetStartFrame()
{
    _data._startFrame = 0;
    ui->pStartTimeTB->hide();
}

void xfDlg::resetEndFrame()
{
    _data._endFrame = _data._frames-1;
    ui->pEndTimeTB->hide();
}

bool xfDlg::eventFilter(QObject *watched, QEvent *event)
{
    if (watched==ui->pFrameDial && event->type()==QEvent::MouseButtonDblClick)
    {
        float posInPerc = ((float)ui->pFrameDial->value()-(float)ui->pFrameDial->minimum())/((float)ui->pFrameDial->maximum() - (float)ui->pFrameDial->minimum());
        float xPos = posInPerc*285.0f+590;

        if (!ui->pStartTimeTB->isVisible())
        {
            _data._startFrame = ui->pFrameDial->value();
            ui->pStartTimeTB->move(xPos,ui->pStartTimeTB->pos().y());
            ui->pStartTimeTB->show();
        }
        else
        {
            if (!ui->pEndTimeTB->isVisible())
            {
                _data._startFrame = ui->pFrameDial->value();
                ui->pEndTimeTB->move(xPos,ui->pEndTimeTB->pos().y());
                ui->pEndTimeTB->show();
            }
            else
            {

            }
        }
    }

    if (event->type()==QEvent::KeyPress)
    {
        QKeyEvent *pKEvent = dynamic_cast<QKeyEvent*>(event);
        if (pKEvent && pKEvent->modifiers()==Qt::ControlModifier)
        {
            switch (pKEvent->key())
            {
            case Qt::Key_I : // import
                ui->pTabWdgt->setCurrentIndex(0);
                ui->pInputTB->animateClick();
                break;
            case Qt::Key_S : // save results
                ui->pTabWdgt->setCurrentIndex(2);
                ui->pExportTB->animateClick();
                break;
            case Qt::Key_C : // calculate
                ui->pTabWdgt->setCurrentIndex(0);
                ui->pStartTB->animateClick();
                break;
            case Qt::Key_P : // play / stop
                ui->pTabWdgt->setCurrentIndex(0);
                ui->pPlayTB->animateClick();
                break;
            default:
                break;
            }
        }
    }
    return QDialog::eventFilter(watched,event);
}

void xfDlg::start()
{
    if (_data._dataValid)
    {
        _data._rotation ? calculate3DXLF() : calculate2DXLF();
    }
}

void xfDlg::abortCurrentFunction()
{
    emit MSG("function aborted",true);
    _abb = true;
}

void xfDlg::rotationModeOnOff(bool b)
{
    _data._rotation = b;
    displayFrame(ui->pFrameDial->value());
}

void xfDlg::dispNextFrame()
{
    if (_data._dataValid)
    {
        int f = (ui->pFrameDial->value()+1) % _data._frames;
        ui->pFrameDial->setValue(f);
    }
}

void xfDlg::dispFrame()
{
    displayFrame(ui->pFrameDial->value());
}

void xfDlg::startStopPlayTime(bool b)
{
    if (_data._dataValid)
    {
        float fps = (float)_data._frames/(*_data.pTotalTime);
        b ? pPlayTimer->start(1000.0f / fps) : pPlayTimer->stop();
    }
}

void xfDlg::updateAndDisplayStatus()
{
    QPixmap error(":/images/status_error.png");
    error = error.scaledToHeight(40);

    if (_errorMsgCount>0)
    {
        QPixmap pix(":/images/message_button.png");
        QPainter pain(&pix);
        QTransform trans;
        trans=trans.rotate(90);
        pain.setTransform(trans);
        trans.reset();
        pain.setTransform(trans.translate(0,-100),true);
        pain.drawPixmap(0,0,error);
        QFont f;
        f=ui->pTabWdgt->font();
        f.setPixelSize(20);
        pain.setFont(f);
        pain.drawText(QRect(27,50,50,30),Qt::AlignRight,QString("%1").arg(_errorMsgCount));
        pain.end();
        ui->pTabWdgt->setTabIcon(2,QIcon(QPixmap(pix)));
        ui->pTabWdgt->update();
    }
    else
    {
        QPixmap pix(":/images/message_button.png");
        ui->pTabWdgt->setTabIcon(2,QIcon(QPixmap(pix)));
        ui->pTabWdgt->update();
    }

    if (pChart)
    {
        if (dynamic_cast<QPolarChart*>(pChart))
        {
            ui->pSeriesLab->hide();
            ui->pVisCB->hide();
        }
        else
        {
            ui->pSeriesLab->show();
            ui->pVisCB->show();
        }
    }
}

void xfDlg::MSGSlot(const QString& txt, const bool& error)
{
    QColor col=ui->pMSGBrowser->textColor();
    QFont f = font();
    f.setPixelSize(14);
    ui->pMSGBrowser->setFont(f);
    error ? ui->pMSGBrowser->setTextColor(Qt::red) : ui->pMSGBrowser->setTextColor(col);
    ui->pMSGBrowser->append(txt);
    ui->pMSGBrowser->setTextColor(col);
    if (error && ui->pTabWdgt->currentIndex()!=2) {
        ++_errorMsgCount;
        updateAndDisplayStatus();
    }
}

void xfDlg::import()
{
    xfImportDlg dlg(0);
    if (!_data._fileName.isEmpty())
    {
        QFileInfo info(_data._fileName);
        dlg.setCurrentFile(info.absolutePath());
    }
    connect(&dlg,SIGNAL(MSG(const QString&,const bool&)),this,SLOT(MSGSlot(const QString&,const bool&)));
    connect(&dlg,SIGNAL(selectedTIFFile(const QString&)),this,SLOT(selectedImportFile(const QString&)));
    dlg.exec();
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
        _data._rotation = true;
    else
    {
        *_data.pTotalTime = 34.0;
        ui->pTotalTimeLCD->display(*_data.pTotalTime);
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

    createStandardPathItems();
    updateAndDisplayStatus();

    if (_data._rotation && !ui->pTotalAngleTB->isChecked()) ui->pTotalAngleTB->animateClick();
    if (!_data._rotation && ui->pTotalAngleTB->isChecked()) ui->pTotalAngleTB->animateClick();

}

void xfDlg::updateClock()
{
    ui->pTimeLab->setText(QDateTime::currentDateTime().toString("dd.MMMM.yyyy hh:mm:ss"));
}

xfDlg::~xfDlg()
{
    delete ui;
}

