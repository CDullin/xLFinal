#ifndef XFPROGRESSDLG_H
#define XFPROGRESSDLG_H

#include <QDialog>

namespace Ui {
class xfProgressDlg;
}

class xfProgressDlg : public QDialog
{
    Q_OBJECT

public:
    explicit xfProgressDlg(QWidget *parent = nullptr);
    ~xfProgressDlg();
    void setRange(int,int);
    void setValue(int);

signals:
    void aborted();

private:
    Ui::xfProgressDlg *ui;
};

#endif // XFPROGRESSDLG_H
