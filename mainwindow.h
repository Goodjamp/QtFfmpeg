#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>

#include <stdint.h>

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
    void pbCameraPlayClick(bool click);
    void pbFilePlayClick(bool click);
    void pbCameraRecordClick(bool click);
    void pbStopClick(bool click);
    void readFrameTimeoute();
    void pbStreamNetworkClick(bool click);

private:
    typedef enum {
        CAMERA_PLAY,
        FILE_PLAY,
        CAMERA_RECORD,
        STREAM_NETWORK,
    } ActivityType;

    Ui::MainWindow *ui;

    FFmpegDecode *decodeItem;

    QLineEdit *leFilePath;
    QPushButton *pbCameraPlay;
    QPushButton *pbFilePlay;
    QPushButton *pbCameraRecord;
    QPushButton *pbStreamNetwork;
    QPushButton *pbStop;
    QVBoxLayout *vblL;
    QLabel *lDisplay;
    QTimer *framePerioTimer;
    bool isRun = false;
    ActivityType activityType;
};
#endif // MAINWINDOW_H
