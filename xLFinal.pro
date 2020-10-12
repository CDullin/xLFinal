QT       += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../../3rd_party/Alglib/src/alglibinternal.cpp \
    ../../3rd_party/Alglib/src/alglibmisc.cpp \
    ../../3rd_party/Alglib/src/ap.cpp \
    ../../3rd_party/Alglib/src/dataanalysis.cpp \
    ../../3rd_party/Alglib/src/diffequations.cpp \
    ../../3rd_party/Alglib/src/fasttransforms.cpp \
    ../../3rd_party/Alglib/src/integration.cpp \
    ../../3rd_party/Alglib/src/interpolation.cpp \
    ../../3rd_party/Alglib/src/linalg.cpp \
    ../../3rd_party/Alglib/src/optimization.cpp \
    ../../3rd_party/Alglib/src/solvers.cpp \
    ../../3rd_party/Alglib/src/specialfunctions.cpp \
    ../../3rd_party/Alglib/src/statistics.cpp \
    lcdconnector.cpp \
    main.cpp \
    xf_tools.cpp \
    xfdlg.cpp \
    xfdlg_2D_calculation.cpp \
    xfdlg_3D_calculation.cpp \
    xfdlg_dataDisplay.cpp \
    xfdlg_io.cpp \
    xfimportdlg.cpp \
    xfinsertnrdlg.cpp \
    xfmsgdlg.cpp \
    xfprogressdlg.cpp \
    xfquestiondlg.cpp \
    xlflunglobevisualization.cpp

HEADERS += \
    ../../3rd_party/Alglib/src/alglibinternal.h \
    ../../3rd_party/Alglib/src/alglibmisc.h \
    ../../3rd_party/Alglib/src/ap.h \
    ../../3rd_party/Alglib/src/dataanalysis.h \
    ../../3rd_party/Alglib/src/diffequations.h \
    ../../3rd_party/Alglib/src/fasttransforms.h \
    ../../3rd_party/Alglib/src/integration.h \
    ../../3rd_party/Alglib/src/interpolation.h \
    ../../3rd_party/Alglib/src/linalg.h \
    ../../3rd_party/Alglib/src/optimization.h \
    ../../3rd_party/Alglib/src/solvers.h \
    ../../3rd_party/Alglib/src/specialfunctions.h \
    ../../3rd_party/Alglib/src/statistics.h \
    ../../3rd_party/Alglib/src/stdafx.h \
    lcdconnector.h \
    xf_tools.h \
    xf_types.h \
    xfdlg.h \
    xfimportdlg.h \
    xfinsertnrdlg.h \
    xfmsgdlg.h \
    xfprogressdlg.h \
    xfquestiondlg.h \
    xlflunglobevisualization.h

FORMS += \
    xfdlg.ui \
    xfimportdlg.ui \
    xfinsertnrdlg.ui \
    xfmsgdlg.ui \
    xfprogressdlg.ui \
    xfquestiondlg.ui

TRANSLATIONS += \
    xLFinal_en_US.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    xf_resources.qrc

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../3rd_party/tiff-4.0.10/libtiff/release/ -ltiff
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../3rd_party/tiff-4.0.10/libtiff/debug/ -ltiff
else:unix: LIBS += -L$$PWD/../../3rd_party/tiff-4.0.10/libtiff/ -ltiff

INCLUDEPATH += $$PWD/../../3rd_party/tiff-4.0.10/libtiff
DEPENDPATH += $$PWD/../../3rd_party/tiff-4.0.10/libtiff
