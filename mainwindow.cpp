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

    pbStreamNetwork = new QPushButton(this);
    pbStreamNetwork->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pbStreamNetwork->setMinimumSize(0, 30);
    pbStreamNetwork->setMaximumSize(1000, 30);
    pbStreamNetwork->setText("Strem network");


    pbStop = new QPushButton(this);
    pbStop->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pbStop->setMinimumSize(0, 30);
    pbStop->setMaximumSize(1000, 30);
    pbStop->setText("Stop");

    lDisplay = new QLabel();

    vblL = new QVBoxLayout();
    centralWidget()->setLayout(vblL);
    vblL->insertWidget(0, leFilePath);
    vblL->insertWidget(1, pbCameraPlay);
    vblL->insertWidget(2, pbFilePlay);
    vblL->insertWidget(3, pbCameraRecord);
    vblL->insertWidget(4, pbStreamNetwork);
    vblL->insertWidget(5, pbStop);
    vblL->insertWidget(5, lDisplay);
    vblL->insertSpacerItem(6, new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

    framePerioTimer = new QTimer(this);

    connect(pbCameraPlay, &QPushButton::clicked, this, &MainWindow::pbCameraPlayClick);
    connect(pbFilePlay, &QPushButton::clicked, this, &MainWindow::pbFilePlayClick);
    connect(pbCameraRecord, &QPushButton::clicked, this, &MainWindow::pbCameraRecordClick);
    connect(pbStreamNetwork, &QPushButton::clicked, this, &MainWindow::pbStreamNetworkClick);
    connect(pbStop, &QPushButton::clicked, this, &MainWindow::pbStopClick);
    connect(framePerioTimer, &QTimer::timeout, this, &MainWindow::readFrameTimeoute);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::pbCameraPlayClick(bool click)
{
    activityType = CAMERA_PLAY;
    decodeItem->camerraPlay(QSize(320, 240));
    lDisplay->resize(decodeItem->getFrameSize());

    isRun = true;
    framePerioTimer->setInterval(1000/30);
    framePerioTimer->start();
}

void MainWindow::pbFilePlayClick(bool click)
{
    activityType = FILE_PLAY;
    decodeItem->filePlay();
    lDisplay->resize(decodeItem->getFrameSize());

    isRun = true;
    framePerioTimer->setInterval(1000/30);
    framePerioTimer->start();
}

void MainWindow::pbCameraRecordClick(bool click)
{
    qDebug()<<"Init ffmpeg";
    decodeItem->cameraRecord(QSize(320, 240), "/home/oleksandr/camera.mpg4");
    activityType = CAMERA_RECORD;
    //decodeItem->main();
    //lDisplay->resize(decodeItem->getFrameSize());

    isRun = true;
    framePerioTimer->setInterval(1000/30);
    framePerioTimer->start();
}

void MainWindow::pbStreamNetworkClick(bool click)
{
    qDebug()<<"Init network stream";
    decodeItem->cameraSreamNetwork(QSize(320, 240));
    activityType = STREAM_NETWORK;
    //decodeItem->main();
    //lDisplay->resize(decodeItem->getFrameSize());

    isRun = true;
    framePerioTimer->setInterval(1000/30);
    framePerioTimer->start();
}

void MainWindow::pbStopClick(bool click)
{
    isRun = false;
}

void MainWindow::readFrameTimeoute()
{
    if (isRun == true) {
        QSize frameSize = decodeItem->getFrameSize();
        uint8_t imageBuff[ frameSize.width() * frameSize.height() * 3];
        QPixmap frame;

        switch(activityType) {
        case CAMERA_PLAY:
        case FILE_PLAY:
            decodeItem->readFrame(imageBuff);
            frame = QPixmap::fromImage(QImage(imageBuff, frameSize.width(), frameSize.height(), QImage::Format_RGB888));
            break;

        case CAMERA_RECORD:
            decodeItem->encode(imageBuff);
            frame = QPixmap::fromImage(QImage(imageBuff, frameSize.width(), frameSize.height(), QImage::Format_Grayscale8));
            break;

        case STREAM_NETWORK:
            decodeItem->streamRtp(imageBuff);
            frame = QPixmap::fromImage(QImage(imageBuff, frameSize.width(), frameSize.height(), QImage::Format_Grayscale8));
            break;
        }

        lDisplay->setPixmap(frame);
    } else {

    }
}
