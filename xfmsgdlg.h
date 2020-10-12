#ifndef XFMSGDLG_H
#define XFMSGDLG_H

#include <QDialog>

namespace Ui {
class xfMSGDlg;
}

class xfMSGDlg : public QDialog
{
    Q_OBJECT

public:
    explicit xfMSGDlg(const QString& txt,QWidget *parent = nullptr);
    ~xfMSGDlg();

protected:
    void keyPressEvent(QKeyEvent *) override;

private:
    Ui::xfMSGDlg *ui;
};

#endif // XFMSGDLG_H
