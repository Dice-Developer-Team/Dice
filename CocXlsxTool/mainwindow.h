#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
	void dragEnterEvent(QDragEnterEvent *e);
	void dropEvent(QDropEvent *e);

private slots:
    void on_pushButton_clicked();

    void on_lineEdit_textChanged(const QString &arg1);

    void on_pushButton_export_clicked();

    void on_pushButton_import_clicked();

    void on_checkBox_2_clicked(bool checked);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
