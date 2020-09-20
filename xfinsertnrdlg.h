#ifndef XFINSERTNRDLG_H
#define XFINSERTNRDLG_H

#include <QDialog>

namespace Ui {
class xfInsertNrDlg;
}

class xfInsertNrDlg : public QDialog
{
    Q_OBJECT

public:
    explicit xfInsertNrDlg(const float& nr,QWidget *parent = nullptr);
    ~xfInsertNrDlg();

protected:
    bool eventFilter(QObject *, QEvent *) override;

protected slots:
    void BtnClicked();
    virtual void accept() override;

signals:
    void nr(const float&);

private:
    Ui::xfInsertNrDlg *ui;
};

#endif // XFINSERTNRDLG_H
