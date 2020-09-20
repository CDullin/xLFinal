#ifndef XFIMPORTDLG_H
#define XFIMPORTDLG_H

#include <QDialog>
#include <QListWidget>

namespace Ui {
class xfImportDlg;
}

class xfImportDlg : public QDialog
{
    Q_OBJECT

public:
    explicit xfImportDlg(QWidget *parent = nullptr);
    ~xfImportDlg();
    void setCurrentFile(const QString&);

public slots:
    void show();
    void listTiffFilesInCurrentDir(const QModelIndex&);
    void itemActivated(QListWidgetItem *item);
    virtual void accept() override;

signals:
    void MSG(const QString& txt,const bool& error=false);
    void selectedTIFFile(const QString&);

private:
    Ui::xfImportDlg *ui;
};

#endif // XFIMPORTDLG_H
