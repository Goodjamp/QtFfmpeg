#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QDebug>
#include <QSpacerItem>
#include <QImage>
#include <QPixmap>
#include <QThread>
#include <QFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    decodeItem = new FFmpegDecode();

    leFilePath = new QLineEdit(this);
    leFilePath->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    leFilePath->setMinimumSize(0, 30);
    leFilePath->setMaximumSize(1000, 30);
    leFilePath->setText("D:/Programing/SW/QtFFmpeg/QtFFmpeg/test.mpg");

    pbCameraPlay = new QPushButton(this);
    pbCameraPlay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pbCameraPlay->setMinimumSize(0, 30);
    pbCameraPlay->setMaximumSize(1000, 30);
    pbCameraPlay->setText("Camera play");

    pbFilePlay = new QPushButton(this);
    pbFilePlay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pbFilePlay->setMinimumSize(0, 30);
    pbFilePlay->setMaximumSize(1000, 30);
    pbFilePlay->setText("File play");

    pbCameraRecord = new QPushButton(this);
    pbCameraRecord->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pbCameraRecord->setMinimumSize(0, 30);
    pbCameraRecord->setMaximumSize(1000, 30);
    pbCameraRecord->setText("Camera record");

    lDisplay = new QLabel();

    vblL = new QVBoxLayout();
    centralWidget()->setLayout(vblL);
    vblL->insertWidget(0, leFilePath);
    vblL->insertWidget(1, pbCameraPlay);
    vblL->insertWidget(2, pbFilePlay);
    vblL->insertWidget(3, pbCameraRecord);
    vblL->insertWidget(4, lDisplay);
    vblL->insertSpacerItem(5, new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

    framePerioTimer = new QTimer(this);

    connect(pbCameraPlay, &QPushButton::clicked, this, &MainWindow::pbCameraPlayClick);
    connect(pbFilePlay, &QPushButton::clicked, this, &MainWindow::pbFilePlayClick);
    connect(pbCameraRecord, &QPushButton::clicked, this, &MainWindow::pbCameraRecordClick);
    connect(framePerioTimer, &QTimer::timeout, this, &MainWindow::readFrameTimeoute);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::pbCameraPlayClick(bool click)
{
    qDebug()<<"Init ffmpeg";
    //decodeItem->initDecoder(this->leFilePath->text());
    decodeItem->connectCamerra();
    lDisplay->resize(decodeItem->getFrameSize());

    framePerioTimer->setInterval(1000/30);
    framePerioTimer->start();
}

void MainWindow::pbFilePlayClick(bool click)
{
    qDebug()<<"Init ffmpeg";
    //decodeItem->initDecoder(this->leFilePath->text());
    decodeItem->connectToFile();
    lDisplay->resize(decodeItem->getFrameSize());

    framePerioTimer->setInterval(1000/30);
    framePerioTimer->start();
}

void MainWindow::pbCameraRecordClick(bool click)
{
    qDebug()<<"Init ffmpeg";
    decodeItem->saveCameraStream("/home/oleksandr/camera.mpg4");
    lDisplay->resize(decodeItem->getFrameSize());

    framePerioTimer->setInterval(1000/30);
    framePerioTimer->start();
}

void MainWindow::readFrameTimeoute()
{
    QSize frameSize = decodeItem->getFrameSize();
    uint8_t imageBuff[ frameSize.width() * frameSize.height()];
    QPixmap frame;

    decodeItem->readFrame(imageBuff);
    frame = QPixmap::fromImage(QImage(imageBuff, frameSize.width(), frameSize.height(), QImage::Format_Grayscale8));
    lDisplay->setPixmap(frame);
}
