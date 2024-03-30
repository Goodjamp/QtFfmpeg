#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <qDebug>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    decodeItem = new FFmpegDecode();
    QLabel *ffmpegInfo = new QLabel(this);
    ffmpegInfo->setGeometry(20, 20, 200, 20);
    ffmpegInfo->setText(decodeItem->getffmpegInfo());

}

MainWindow::~MainWindow()
{
    delete ui;
}
