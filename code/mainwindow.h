#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLayout>
#include <QMainWindow>
#include <QTimer>
#include <QProgressBar>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>
#include <QDateTime>
#include <QString>

#include "defs.h"
#include "qcustomplot.h" // import, not our work
#include "neureset.h"
#include "agent.h"
#include "databasemanager.h"

QT_BEGIN_NAMESPACE

namespace Ui { class MainWindow; }

QT_END_NAMESPACE

class MainWindow : public QMainWindow{
    Q_OBJECT

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;
        static MainWindow* getInstance();

    private:
        static MainWindow* instance; // Singleton enforced by Qt
        Ui::MainWindow* ui;

        Neureset* const neureset;
        Agent* agent;

        // called once.
        const QVector<double>& domainTime;
        const QVector<double>& ampTime;

        const QVector<double>& domainDFT;
        const QVector<double>& ampDFT;

        const double& peakFreq;
        const double& peakFreqAmp;
        const int& progress;
        const double& treatAmp;
        std::mutex& mtx;

        int site;
        bool isAttached;
        bool isTreat;
        bool isInSession;
        bool isInMenu;
        bool isInHistory;
        bool isPower;

        QDateTime currentDateTime;

        //--------------------------------------------------------------------------------------//

        bool isRunning;
        bool isPause;
        int timeleft;

        QFuture<void> future;
        QTimer* refresh;
        QTimer* battery;
        QTimer* redLight;

        QTimer* fiveMinutes;
        QTimer* deviceClock;
        QTimer* treatmentTimer;

        QCustomPlot* plotFreq;
        QCustomPlot* plotDft;

        DataBaseManager* dbManager;
        DataBaseManager* db;  // moved from treatment

        void loader();
        void menuInit();
        void treatment();
        void calculateTime(int time);

    private slots:
        void power();
        void updateBrainState();
        void siteChange();
        void attach(int state);
        void pause();
        void stopTreatment();
        void menuUp();
        void menuDown();
        void menuOk();
        void restart();
        void menu();
        void batteryLife();
        void chargeBattery();
        void closeEvent(QCloseEvent *event) override;

        void flashRed();
        void flashGreen();

        void fiveMinTimeOut();
        // The naming is required by Qt
        void on_dateTimeEdit_dateTimeChanged(const QDateTime &dateTime);
        void updateTime();
        void timerUpdate();

};
#endif
