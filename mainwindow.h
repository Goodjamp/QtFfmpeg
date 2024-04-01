#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include "ffmpegdecode.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void pbStartClick(bool click);

private:
    Ui::MainWindow *ui;

    FFmpegDecode *decodeItem;

    QLineEdit *leFilePath;
    QPushButton *pbStart;
    QVBoxLayout *vblL;
};
#endif // MAINWINDOW_H
