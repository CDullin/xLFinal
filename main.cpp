#include "xfdlg.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    xfDlg w;
    w.show();
    return a.exec();
}
