#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QDebug>
#include <QLabel>
#include <QSpacerItem>

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
    vblL = new QVBoxLayout();
    centralWidget()->setLayout(vblL);
    vblL->insertWidget(0, leFilePath);
    vblL->insertWidget(1, pbStart);
    vblL->insertSpacerItem(2, new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

    connect(pbStart, &QPushButton::clicked, this, &MainWindow::pbStartClick);
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
}
