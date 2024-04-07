#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QDebug>
#include <QSpacerItem>
#include <QImage>
#include <QPixmap>
#include <QThread>

#define IMAGE_HEIGHT    480
#define IMAGE_WIDTH     640

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

    pbStart = new QPushButton(this);
    pbStart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pbStart->setMinimumSize(0, 30);
    pbStart->setMaximumSize(1000, 30);
    pbStart->setText("START");

    lDisplay = new QLabel();

    lDisplay->resize(QSize(320, 240));
    imageHeight = IMAGE_HEIGHT;
    imageWidth = IMAGE_WIDTH;

    vblL = new QVBoxLayout();
    centralWidget()->setLayout(vblL);
    vblL->insertWidget(0, leFilePath);
    vblL->insertWidget(1, pbStart);
    vblL->insertWidget(2, lDisplay);
    vblL->insertSpacerItem(3, new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

    framePerioTimer = new QTimer(this);

    connect(pbStart, &QPushButton::clicked, this, &MainWindow::pbStartClick);
    connect(framePerioTimer, &QTimer::timeout, this, &MainWindow::readFrameTimeoute);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::pbStartClick(bool click)
{
    qDebug()<<"Init ffmpeg";
    //decodeItem->initDecoder(this->leFilePath->text());
    decodeItem->connectCamerra();

    framePerioTimer->setInterval(20);
    framePerioTimer->start();
}

void MainWindow::readFrameTimeoute()
{
    uint8_t imageBuff[IMAGE_HEIGHT * IMAGE_WIDTH * 3];
    QPixmap frame;

    decodeItem->readFrame(imageBuff);
    frame = QPixmap::fromImage(QImage(imageBuff, imageWidth, imageHeight, QImage::Format_Grayscale8));
    lDisplay->setPixmap(frame);
}
