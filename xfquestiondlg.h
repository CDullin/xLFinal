#ifndef XFQUESTIONDLG_H
#define XFQUESTIONDLG_H

#include <QDialog>

namespace Ui {
class xfQuestionDlg;
}

class xfQuestionDlg : public QDialog
{
    Q_OBJECT

public:
    explicit xfQuestionDlg(const QString&,QWidget *parent = nullptr);
    ~xfQuestionDlg();

protected:
    void keyPressEvent(QKeyEvent *) override;

private:
    Ui::xfQuestionDlg *ui;
};

#endif // XFQUESTIONDLG_H
