#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow *MainWindow::instance = nullptr;


MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow),

        neureset(Neureset::getInstance()),
        agent(new Agent(neureset)),
        //--------------------------------------------------------------------------------------//
        // references to neureset
        domainTime(neureset->getDomainTime()),
        ampTime(neureset->getAmpTime()),

        domainDFT(neureset->getDomainDFT()),
        ampDFT(neureset->getAmpDFT()),

        peakFreq(neureset->getDomFreq()),
        peakFreqAmp(neureset->getPeakFreqAmp()),
        progress(neureset->getProgress()),
        treatAmp(neureset->getTreatAmp()),
        mtx(neureset->getMutex()),
        //--------------------------------------------------------------------------------------//
        site(0),
        isAttached(false),
        isTreat(false),
        isInSession(false),
        isInMenu(true),
        isInHistory(false),
        isPower(false) {

    ui->setupUi(this);
    instance = this;
    dbManager = new DataBaseManager("main");
    loader();
    //update the clock per second
    deviceClock->start(1000);

    // attach the helmet
    agent->helmet();
    isRunning = false;
}


MainWindow::~MainWindow() {

    refresh->stop();
    stopTreatment();

    future.waitForFinished();

    delete db;
    delete dbManager;
    delete neureset;
    delete agent;
    delete ui;

}

// Singleton enforced by Qt
MainWindow *MainWindow::getInstance() {
    return instance;
}

// Initializes the state of the device (constructor not enough)
void MainWindow::loader() {
    // button power
    connect(ui->power, SIGNAL(released()), this, SLOT(power()));                // reset
    // button pause
    connect(ui->pause, SIGNAL(released()), this, SLOT(pause()));                // reset
    // button attach
    connect(ui->attach, SIGNAL(stateChanged(int)), this, SLOT(attach(int)));    // agent
    // button restart
    connect(ui->restart, SIGNAL(released()), this, SLOT(restart()));            // reset
    // button stop
    connect(ui->stop, SIGNAL(released()), this, SLOT(stopTreatment()));         // stop
    //button up
    connect(ui->upButton, SIGNAL(released()), this, SLOT(menuUp()));
    //button down
    connect(ui->downButton, SIGNAL(released()), this, SLOT(menuDown()));
    //button ok
    connect(ui->okButton, SIGNAL(released()), this, SLOT(menuOk()));
    //button menu
    connect(ui->menuButton, SIGNAL(released()), this, SLOT(menu()));

    //Set default device date and time to 2024-01-01 00:00:00
    QString date = dbManager->getDate();
    currentDateTime = QDateTime::fromString(date, "yyyy-MM-dd HH:mm:ss");

    //device clock, update the device time every minute
    deviceClock = new QTimer(this);
    connect(deviceClock, SIGNAL(timeout()), this, SLOT(updateTime()));

    //--------------------------------------------------------------------------------------//
    // timer - update charts
    refresh = new QTimer(this);
    connect(refresh, SIGNAL(timeout()), this, SLOT(updateBrainState()));

    // timer - battery
    ui->batteryProgress->setRange(0, BATTERY_CAPACITY); // 600 seconds = 10 minutes
    ui->batteryProgress->setValue(BATTERY_CAPACITY);
    battery = new QTimer(this);
    connect(battery, SIGNAL(timeout()), this, SLOT(batteryLife()));
    connect(ui->updateBatteryButton, SIGNAL(released()), this, SLOT(chargeBattery()));

    //--------------------------------------------------------------------------------------//
    // freq plot
    plotFreq = new QCustomPlot(this);

    QVBoxLayout* layoutFreq = new QVBoxLayout(ui->freq);
    layoutFreq->addWidget(plotFreq);
    ui->freq->setLayout(layoutFreq);

    plotFreq->addGraph();
    plotFreq->xAxis->setLabel("time");
    plotFreq->yAxis->setLabel("uv");
    plotFreq->replot();
    plotFreq->setVisible(false);

    //--------------------------------------------------------------------------------------//
    // dft plot
    plotDft = new QCustomPlot(this);

    QVBoxLayout* layoutDft = new QVBoxLayout(ui->dft);
    layoutDft->addWidget(plotDft);
    ui->dft->setLayout(layoutDft);

    plotDft->addGraph();
    plotDft->xAxis->setLabel("freq");
    plotDft->yAxis->setLabel("uv");
    plotDft->replot();
    plotDft->setVisible(false);

    //--------------------------------------------------------------------------------------//
    // slider site
    ui->siteSlider->setMinimum(0);
    ui->siteSlider->setMaximum(NUM_BRAIN_SITES - 1);
    connect(ui->siteSlider, &QSlider::valueChanged, this, &MainWindow::siteChange);

    //--------------------------------------------------------------------------------------//
    // progress bar treatment
    ui->progressBar->setValue(0);

    // ui->attach->setCheckable(false);
    ui->attach->setCheckable(true);
    ui->blue->setStyleSheet("background-color: rgb(246, 245, 244);");
    ui->green->setStyleSheet("background-color: rgb(246, 245, 244);");
    ui->red->setStyleSheet("background-color: rgb(246, 245, 244);");

    //make the screen power off
    ui->topBar->setVisible(false);
    ui->listWindow->setVisible(false);
    ui->progressBar->setVisible(false);
    ui->sessionEnd->setVisible(false);
    ui->historyList->setVisible(false);
    ui->setTimePage->setVisible(false);
    ui->dateTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    ui->treatmentTimer->setVisible(false);
    menuInit();
    ui->PCList->setVisible(false);
    ui->PCrecord->setVisible(false);

    //red light timer
    redLight = new QTimer(this);
    connect(redLight, SIGNAL(timeout()), this, SLOT(flashRed()));
    //the timer to record 5 minutes
    fiveMinutes = new QTimer(this);
    fiveMinutes->setSingleShot(true);
    connect(fiveMinutes, SIGNAL(timeout()), this, SLOT(fiveMinTimeOut()));
    ui->tab->setCurrentIndex(0);

    // system is assumped to start at full battery so no error on botton right
    ui->batteryIndicator->setChecked(true);

    treatmentTimer = new QTimer(this);
    connect(treatmentTimer, SIGNAL(timeout()), this, SLOT(timerUpdate()));

    db = new DataBaseManager("thread");
}

/*
    Battery Timer

    10 % warnings occur, treatment stops

    1 % powers off, counter stops

    otherwise, checked good.
*/
void MainWindow::batteryLife() {

    ui->batteryProgress->setValue(ui->batteryProgress->value() - 1);

    if (ui->batteryProgress->value() <= BATTERY_CAPACITY * LOW_BATTERY_SHUTOFF) {
        battery->stop();
        power();
        qDebug() << "BATTERY DEAD";
    } else if (ui->batteryProgress->value() <= BATTERY_CAPACITY * LOW_BATTERY_THRESHOLD) {
        ui->batteryIndicator->setChecked(false);
        ui->batteryProgress->setStyleSheet(LOW_BATTERY_STYLESHEET);
        stopTreatment();
        qDebug() << "VERY LOW BATTERY";
    } else {
        ui->batteryIndicator->setChecked(true);
    }
}

// In fact you can discharge using this too (helps you to test low power)
void MainWindow::chargeBattery() {
    ui->batteryProgress->setValue((ui->chargeSpinBox->value() / 100.0) * BATTERY_CAPACITY);
    qDebug() << ui->chargeSpinBox->value();
    if (ui->batteryProgress->value() > BATTERY_CAPACITY * LOW_BATTERY_THRESHOLD) {
        ui->batteryProgress->setStyleSheet(NORMAL_BATTERY_STYLESHEET);
    }
}


/*
    horizontal slider.

    sets the distinct site for viewing and treatment
*/
void MainWindow::siteChange() {
    ui->siteLabel->setText(QString::number(ui->siteSlider->value()));
    site = ui->siteSlider->value();

    if (isAttached)
        neureset->setSite(site);
}


/*
    Button Power

    GUI. Will halt gui thread

*/
void MainWindow::power() {
    isPower = !isPower;
    if (isPower) {
        refresh->start(REFRESH_PERIOD);
        ui->topBar->setVisible(true);
        ui->listWindow->setVisible(true);
        ui->progressBar->setVisible(false);
        ui->sessionEnd->setVisible(false);
        ui->treatmentTimer->setVisible(false);
        ui->setTimePage->setVisible(false);
        ui->menuList->setCurrentRow(0);
        ui->menuList->setVisible(true);
        isInMenu = true;
        plotFreq->setVisible(true);
        plotDft->setVisible(true);

        ui->historyList->setVisible(false);
        ui->historyList->setCurrentRow(0);

        isPause = false;        // acts as initializer
        battery->start(1000);   // battery start 1 second refresh
    } else {
        ui->topBar->setVisible(false);
        ui->listWindow->setVisible(false);
        isTreat = false;
        refresh->stop();        // stop updating screen
        battery->stop();        // stop battery timer
        fiveMinutes->stop();    //stop the 5 mins timer if it is not attached
        redLight->stop();       //stop flashing red even if not attached

        ui->power->setChecked(false); //we need this to turn off the power light when battery dead
        ui->maxFreq->setText(0);
        ui->maxFreqAmp->setText(0);


        //stop the treatment if the device is poweroff when a treatment is running
        if (future.isRunning()) {
            treatmentTimer->stop();
            neureset->stopTreatment();
            future.cancel();
            isTreat = false;
        }
        restart(); //clears a pause condition if exists
        //turn off all the lights anyways
        ui->blue->setStyleSheet("background-color: rgb(246, 245, 244);");
        ui->red->setStyleSheet("background-color: rgb(246, 245, 244);");
        ui->green->setStyleSheet("background-color: rgb(246, 245, 244);");

        plotFreq->setVisible(false);
        plotDft->setVisible(false);
        isInHistory = false;
    }
}

/*
    Button Pause

    GUI. Simulation  thread still runs.
*/
void MainWindow::pause() {
    if (!isPause)
        isPause = neureset->togglePause();
}

// To restart the treatment after pause or disconnection
void MainWindow::restart() {
    if (isPause && (isAttached || !isTreat)) //for restart and clears a pause condition when not cantact
        isPause = neureset->togglePause();
}

// if running and want to stop
void MainWindow::stopTreatment() {
    if (future.isRunning()) {
        treatmentTimer->stop();
        //if stop when not attached
        fiveMinutes->stop();
        redLight->stop();
        neureset->stopTreatment();
        future.cancel();
        isTreat = false;
        ui->treatmentTimer->setVisible(false);
        ui->sessionEnd->setVisible(true);

        isInSession = false;

        if (isPause)
            restart();
    }
}


/*
    Button Attach

    Default site is 0 so it exists.

    There was a weird edge case where battery ends and the contact is true but there was no signal
*/
void MainWindow::attach(int state) {

    if (state == Qt::Checked) {
        isAttached = true;
        neureset->setSite(site);
        if (isInSession) {
            redLight->stop();
            ui->red->setStyleSheet("background-color: rgb(246, 245, 244);");
            ui->blue->setStyleSheet("background-color: blue;");

            if (isPause){
                fiveMinutes->stop();
            }
        }
    } else {
        isAttached = false;
        if (isInSession) {
            fiveMinutes->start(300000);
            ui->blue->setStyleSheet("background-color: rgb(246, 245, 244);");
            redLight->start(500);
            pause(); // is in session and disconnected pause.
        }
        neureset->setSite(-1);
    }
}


/*
    Button Treatment

    For database stuff we could pass a string here
    or do exporting from this class before
*/
void MainWindow::treatment() {

    if (!future.isRunning()) {

        treatmentTimer->start(1000);
        timeleft = TREATMENT_TIME;
        double preOverall = neureset->getOverallBaseline();
        QString currentTime;

        //save the current session to database
        currentTime = currentDateTime.toString("yyyy-MM-dd HH:mm:ss");
        dbManager->addSession(currentTime);
        isTreat = true;
        ui->siteSlider->setEnabled(false);

        future = QtConcurrent::run([this, currentTime, preOverall]() {

            double preTreat;
            double postTreat;
            for (int i = 0; i < NUM_BRAIN_SITES; ++i) {

                if (!isTreat) // end treatment
                    break;

                ui->siteSlider->setValue(i);
                site = ui->siteSlider->value();
                ui->siteLabel->setText(QString::number(site));

                // new site, get next treatment location
                neureset->setSite(site);

                // peakFreq is automatically updated, no need to call geter
                // preTreat = neureset->getDomFreq();
                preTreat = peakFreq;

                neureset->treatment(); // complete round 4 shots, 1 site

                // postTreat = neureset->getDomFreq();
                postTreat = peakFreq;

                //add the pre treatment dominant frequenccy to database
                db->addBaseline(i + 1, preTreat, postTreat, currentTime);
            }

            // finished
            QMetaObject::invokeMethod(this, [this, currentTime, preOverall]() {
                // if stopped when red light is on, clear red
                redLight->stop();
                ui->red->setStyleSheet("background-color: rgb(246, 245, 244);");
                ui->treatmentTimer->setVisible(false);
                if (isTreat)
                    ui->sessionEnd->setVisible(true);
                isTreat = false;
                double postOverall = neureset->getOverallBaseline();
                if (!isAttached)
                    neureset->setSite(-1);
                db->addBaseline(-1, preOverall, postOverall, currentTime);
                ui->siteSlider->setValue(neureset->getSite());
                ui->siteSlider->setEnabled(true);
                isInSession = false;

            }, Qt::QueuedConnection);
        });
    }
}


/*
    Update brain state on screen.

    Size of input plot arrays must be the same.
*/
void MainWindow::updateBrainState() {
    // break out of current update if job isn't done.
    if (isRunning)
        return;

    isRunning = true;

    // protects from data being read and modified at same time
    if (mtx.try_lock()) {

        neureset->generator();

        // freq
        plotFreq->graph(0)->setData(domainTime, ampTime);  // needs to be QVector<double>, QVector<double>
        plotFreq->rescaleAxes();
        plotFreq->replot(QCustomPlot::rpQueuedReplot);

        // dft
        plotDft->graph(0)->setData(domainDFT, ampDFT);
        plotDft->rescaleAxes();
        plotDft->replot(QCustomPlot::rpQueuedReplot);

        //--------------------------------------------------------------------------------------//

        ui->maxFreq->setText(QString::number(peakFreq));
        ui->maxFreqAmp->setText(QString::number(peakFreqAmp));

        //--------------------------------------------------------------------------------------//

        ui->progressBar->setValue(100 * progress / (NUM_OFFSETS * NUM_BRAIN_SITES));

        if (treatAmp > 0.)
            flashGreen();
        else
            ui->green->setStyleSheet("background-color: rgb(246, 245, 244);");

        mtx.unlock();
    }

    isRunning = false;
}


//Initiate the menu
void MainWindow::menuInit() {
    ui->menuList->addItems({"NEW SESSION", "SET TIME", "HISTORY", "UPLOAD TO PC"});
    ui->menuList->setCurrentRow(0);
    ui->menuList->setAttribute(Qt::WA_TransparentForMouseEvents);
}

void MainWindow::menuUp() {
    if (!isInHistory) {
        if (ui->menuList->currentRow() - 1 >= 0)
            ui->menuList->setCurrentRow(ui->menuList->currentRow() - 1);
    } else {
        if (ui->historyList->currentRow() - 1 >= 0)
            ui->historyList->setCurrentRow(ui->historyList->currentRow() - 1);
    }
}

void MainWindow::menuDown() {
    if (!isInHistory) {
        if (ui->menuList->currentRow() + 1 < ui->menuList->count())
            ui->menuList->setCurrentRow(ui->menuList->currentRow() + 1);
    } else {
        if (ui->historyList->currentRow() + 1 < ui->historyList->count())
            ui->historyList->setCurrentRow(ui->historyList->currentRow() + 1);
    }
}

// When you select an option
void MainWindow::menuOk() {
    if (isInMenu) {
        isInMenu = false;
        QVector <QString> slist;
        QStringList strList;
        switch (ui->menuList->currentRow()) {
            case 0: { //start a new session
                if (isAttached){
                    ui->tab->setCurrentIndex(0);
                    ui->PCList->setVisible(false);
                    ui->PCrecord->setVisible(false);
                    neureset->resetProgress();
                    isInSession = true;
                    ui->menuList->setVisible(false);
                    ui->progressBar->setVisible(true);
                    calculateTime(TREATMENT_TIME);
                    ui->treatmentTimer->setVisible(true);
                    ui->sessionEnd->setVisible(false);
                    ui->blue->setStyleSheet("background-color: blue;");
                    treatment();

                } else
                    isInMenu = true; // not attached, lets the menu work again
                break;
            }
            case 1: { //set time
                ui->tab->setCurrentIndex(0);
                ui->PCList->setVisible(false);
                ui->PCrecord->setVisible(false);
                ui->menuList->setVisible(false);
                ui->dateTimeEdit->setDateTime(currentDateTime);
                ui->setTimePage->setVisible(true);
                break;
            }
            case 2: { //view history
                isInHistory = true;
                ui->tab->setCurrentIndex(0);
                ui->PCList->setVisible(false);
                ui->PCrecord->setVisible(false);
                slist = dbManager->getSession();
                ui->historyList->clear();
                strList = QStringList::fromVector(slist);
                ui->historyList->addItems(strList);
                ui->historyList->setCurrentRow(0);
                ui->historyList->setAttribute(Qt::WA_TransparentForMouseEvents);
                ui->historyList->setVisible(true);

                break;
            }
            case 3: {//up load to PC
                ui->PCList->setVisible(true);
                ui->PCrecord->setVisible(true);
                ui->PCrecord->clear();
                QStringList records;
                QVector < SiteInfo * > sites;
                isInMenu = true;
                ui->tab->setCurrentIndex(1); // Switches to the PC end
                slist = dbManager->getSession();
                if (!slist.empty()) {
                    for (int i = 0; i < slist.size(); ++i) {
                        QString date = slist[i];
                        QString singleRecord = "Treatment date: " + date + "\n";
                        sites = dbManager->getSiteRecords(date);
                        //add the overall baseline to the qstring.
                        int size = sites.size();
                        if (size > 0) {
                            //if the treatment runs till end, the last site record will be the overall baseline
                            if (sites.last()->getSite() == -1) {
                                singleRecord += sites.last()->toString();
                                size -= 1;
                            }
                            //loop through the sites
                            for (int j = 0; j < size; ++j) {
                                singleRecord += sites[j]->toString();
                            }
                            records += singleRecord;
                        }
                    }
                }
                ui->PCrecord->addItems(records);
                ui->PCrecord->setCurrentRow(0);
                break;
            }
            default:
                break;
        }
    }
}

// Display the menu
void MainWindow::menu() {
    isInMenu = true;
    isInHistory = false;
    //turn off all the lights anyways
    ui->blue->setStyleSheet("background-color: rgb(246, 245, 244);");
    ui->red->setStyleSheet("background-color: rgb(246, 245, 244);");
    ui->green->setStyleSheet("background-color: rgb(246, 245, 244);");
    if (!isTreat) {
        ui->menuList->setVisible(true);
        ui->setTimePage->setVisible(false);
        ui->progressBar->setVisible(false);
        ui->sessionEnd->setVisible(false);
        ui->treatmentTimer->setVisible(false);
        ui->historyList->setVisible(false);
    }
}

// These functions handle light flashing
void MainWindow::flashGreen() {
    static bool isGreenOn;
    if (isGreenOn)
        ui->green->setStyleSheet("background-color: green;");
    else
        ui->green->setStyleSheet("background-color: rgb(246, 245, 244);");
    isGreenOn = !isGreenOn;
}

void MainWindow::flashRed() {
    static bool isRedOn;
    if (isRedOn) {
        ui->red->setStyleSheet("background-color: red;");
    } else {
        ui->red->setStyleSheet("background-color: rgb(246, 245, 244);");
    }
    isRedOn = !isRedOn;
}

// Turns the device off after 5 minutes of disconnection
void MainWindow::fiveMinTimeOut() {
    redLight->stop();
    ui->red->setStyleSheet("background-color: rgb(246, 245, 244);");
    power();
}

// This function name is intentional (Qt wants it!!!)
void MainWindow::on_dateTimeEdit_dateTimeChanged(const QDateTime &dateTime) {
    currentDateTime = dateTime;
}

void MainWindow::updateTime() {
    currentDateTime = currentDateTime.addSecs(1);
}

// countdown time on treatment
void MainWindow::timerUpdate() {
    if (!isPause) {
        timeleft--;
        calculateTime(timeleft);

        if (timeleft <= 0) {
            treatmentTimer->stop();
        }
    }
}

// displays the timer
void MainWindow::calculateTime(int time) {
    int minutes = time / 60;
    int seconds = time % 60;
    ui->treatmentTimer->setText(QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')));
    QFont font = ui->treatmentTimer->font();
    font.setPointSize(22);
    ui->treatmentTimer->setFont(font);
}

//SAVE THE date and time to database when program is off
void MainWindow::closeEvent(QCloseEvent *event) {
    QString currentTime = currentDateTime.toString("yyyy-MM-dd HH:mm:ss");
    dbManager->updateDate(currentTime);
}

