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

    pbStart = new QPushButton(this);
    pbStart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pbStart->setMinimumSize(0, 30);
    pbStart->setMaximumSize(1000, 30);
    pbStart->setText("START");

    lDisplay = new QLabel();

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
    //decodeItem->connectToFile();
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
