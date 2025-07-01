#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "console.h"
#include "settingsdialog.h"

#include <QMessageBox>
#include <QLabel>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <QProgressDialog>

#define timeOut 2000

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    console = new Console;
    console->setEnabled(false);
    setCentralWidget(console);
    serial = new QSerialPort(this);
    settings = new SettingsDialog;

    // Ui Init
    ui->actionRefresh->setEnabled(false);
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionConfigure->setEnabled(true);
    ui->actionClear->setEnabled(false);
    ui->actionClose->setEnabled(true);

    ui->actionEnter->setEnabled(false);
    ui->actionSentroot->setEnabled(false);

    ui->actionSerial->setEnabled(true);
    ui->actionSetSlaveTime->setEnabled(false);


    ui->actionAutoTest->setEnabled(true);

    status = new QLabel;
    ui->statusBar->addWidget(status);

    initActionsConnections();

    connect(serial, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
            this, &MainWindow::handleError);
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(console, &Console::getData, this, &MainWindow::writeData);

    // open and refresh serialport automaticly
    openSerialPort();
    autoRefreshTimer= new QTimer(this);
    connect(autoRefreshTimer,SIGNAL(timeout()),this,SLOT(reFresh()));
    autoRefreshTimer->start(5000);

    // set default board and update it after client send this message
    // check readData()
    testDevice = "IMX6QDLUL";
}

MainWindow::~MainWindow()
{
    delete settings;
    delete ui;
}

void MainWindow::Delay_MSec_Suspend(unsigned int msec)
{
    QTime _Timer = QTime::currentTime();

    QTime _NowTimer;
    do{
              _NowTimer=QTime::currentTime();
    }while (_Timer.msecsTo(_NowTimer)<=msec);
}

void MainWindow::Delay_MSec(unsigned int msec)
{
    QTime _Timer = QTime::currentTime().addMSecs(msec);

    while( QTime::currentTime() < _Timer )

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void MainWindow::setTestDevice(QString &device)
{
    testDevice = device;
}

void MainWindow::openSerialPort()
{
    SettingsDialog::Settings p = settings->settings();
    serial->setPortName(p.name);
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);
    if (serial->open(QIODevice::ReadWrite)) {
        console->setEnabled(true);
        console->setLocalEchoEnabled(p.localEchoEnabled);
        ui->actionRefresh->setEnabled(true);
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(true);
        ui->actionConfigure->setEnabled(false);
        ui->actionClear->setEnabled(true);

        ui->actionSentroot->setEnabled(true);
        ui->actionEnter->setEnabled(true);
        ui->actionSetSlaveTime->setEnabled(true);


        showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                          .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                          .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));

    } else {
        QMessageBox::critical(this, tr("Error"), serial->errorString());

        showStatusMessage(tr("Open error"));
    }

    // We will receive this value from client, not fix this in master.
    //setTestDevice(p.testDevice);
}

void MainWindow::manOpenSerialPort()
{
    openSerialPort();
    autoRefreshTimer->start(5000);
}

void MainWindow::closeSerialPort()
{
    if (serial->isOpen())
        serial->close();
    console->setEnabled(false);
    ui->actionRefresh->setEnabled(false);
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionConfigure->setEnabled(true);

    ui->actionSentroot->setEnabled(false);
    ui->actionEnter->setEnabled(false);
    ui->actionSetSlaveTime->setEnabled(false);


    showStatusMessage(tr("Disconnected"));
}

void MainWindow::manCloseSerialPort()
{
    autoRefreshTimer->stop();
    closeSerialPort();
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Hardwaretest_master"),
                       tr("<b>Hardwaretest_master v4.5 </b><br><br> The <b>Hardwaretest_master</b> used as chipsee hardwaretest master, "
                          "it works with hardwaretest_slave to test chipsee devices."));
}

void MainWindow::writeData(const QByteArray &data)
{
    serial->write(data);
}

void MainWindow::readData()
{
    QByteArray data = serial->readAll();
    qDebug() << QString(data);
    console->putData(data);

    // Auto Play Audio
    if(QString(data).startsWith("@@AUDIO",Qt::CaseSensitive)){
        playAudio();
    }

    // Set Device
    if(QString(data).startsWith("@@CS",Qt::CaseSensitive)){
        QString str = QString(data).mid(2).remove(QChar('\n'),Qt::CaseInsensitive);
        qDebug() << str;
        if(str == "CS12800RA4101B")
            str = "CS12800RA4101BOX";
        setTestDevice(str);
    }

    if(QString(data).contains("@@CS12800RA4101",Qt::CaseSensitive)){
        QString str = "CS12800RA4101";
        setTestDevice(str);
    }

    if(QString(data).contains("@@CS12800RA4101P",Qt::CaseSensitive)){
        QString str = "CS12800RA4101P";
        setTestDevice(str);
    }

    if(QString(data).contains("@@CS12800R101P",Qt::CaseSensitive)){
        QString str = "CS12800R101P";
        setTestDevice(str);
    }

    if(QString(data).startsWith("@@LRRA4",Qt::CaseSensitive)){
        QString str = QString(data).mid(2).remove(QChar('\n'),Qt::CaseInsensitive);
        qDebug() << str;
        setTestDevice(str);
    }

    if(QString(data).startsWith("@@AM335",Qt::CaseSensitive)){
            QString str = QString(data).mid(2).remove(QChar('\n'),Qt::CaseInsensitive);
            qDebug() << str;
            setTestDevice(str);
    }

    if(QString(data).contains("@@RK3568",Qt::CaseSensitive)){
            QString str ="RK3568";
            setTestDevice(str);
    }

    if(QString(data).contains("@@RK3588",Qt::CaseSensitive)){
            QString str ="RK3588";
            setTestDevice(str);
    }

    if(QString(data).contains("@@IMX8MP",Qt::CaseSensitive)){
            QString str ="IMX8MP";
            setTestDevice(str);
    }

    if(QString(data).contains("@@STMP25",Qt::CaseSensitive)){
            QString str ="STMP25";
            setTestDevice(str);
    }

    //ZIGBEE
    if(testDevice == "CS10600RA4070" || testDevice == "CS10600RA4070D" || testDevice == "CS12800RA4101" || testDevice == "LRRA4-101" || testDevice == "CS12800RA4101BOX" || testDevice == "CS12800RA4101A" || testDevice == "CS12800RA4101P" || testDevice == "CS19108RA4133P" || testDevice == "CS10768RA4150P" || testDevice == "CS19108RA4156P" || testDevice == "CS19108RA4215P" || testDevice == "CS12800PX101"){
        if(QString(data).startsWith("ZIGBEEISOK",Qt::CaseSensitive)){
            QString cmdstr = "echo Zigbee is OK > /tmp/zigbee.txt";
            system(cmdstr.toLocal8Bit());
        }

        if(QString(data).startsWith("ZIGBEEISNOTOK",Qt::CaseSensitive)){
            QString cmdstr = "echo Zigbee is Not OK > /tmp/zigbee.txt";
            system(cmdstr.toLocal8Bit());
        }
    }else{
        QString cmdstr = "echo > /tmp/zigbee.txt";
        system(cmdstr.toLocal8Bit());
    }

    // RTC
    if(QString(data).startsWith("RTCISOK",Qt::CaseSensitive)){
        QString cmdstr = "echo RTC is OK > /tmp/rtc.txt";
        system(cmdstr.toLocal8Bit());
    }

    if(QString(data).contains("RTCISOK",Qt::CaseSensitive)){
        QString cmdstr = "echo RTC is OK > /tmp/rtc.txt";
        system(cmdstr.toLocal8Bit());
    }

    if(QString(data).startsWith("RTCISNOTOK",Qt::CaseSensitive)){
        QString cmdstr = "echo RTC is Not OK > /tmp/rtc.txt";
        system(cmdstr.toLocal8Bit());
    }

    if(QString(data).contains("RTCISNOTOK",Qt::CaseSensitive)){
        QString cmdstr = "echo RTC is Not OK > /tmp/rtc.txt";
        system(cmdstr.toLocal8Bit());
    }

    //CS12800RA101 ttyUSB0 ttyUSB1
    if(testDevice == "CS12800RA101"){
        if(QString(data).startsWith("TTYUSBISOK",Qt::CaseSensitive)){
            QString cmdstr = "echo ttyUSB0 OK. > /tmp/serial23.txt";
            system(cmdstr.toLocal8Bit());
            cmdstr = "echo ttyUSB1 OK. >> /tmp/serial23.txt";
            system(cmdstr.toLocal8Bit());
        }

        if(QString(data).startsWith("TTYUSBISNOK",Qt::CaseSensitive)){
            QString cmdstr = "echo ttyUSB0 NOK. > /tmp/serial23.txt";
            system(cmdstr.toLocal8Bit());
            cmdstr = "echo ttyUSB1 NOK. >> /tmp/serial23.txt";
            system(cmdstr.toLocal8Bit());
        }
    }

    //CS12800R101P ttyS2
    if(testDevice == "CS12800R101P"){
        if(QString(data).contains("TTYS2ISOK",Qt::CaseSensitive)){
            QString cmdstr = "echo ttyS2 OK. > /tmp/ttys2.txt";
            system(cmdstr.toLocal8Bit());
        }
        if(QString(data).contains("TTYS2ISNOK",Qt::CaseSensitive)){
            QString cmdstr = "echo ttyS2 NOK. > /tmp/ttys2.txt";
            system(cmdstr.toLocal8Bit());
        }
    }

    //if(QString(data).startsWith("@@STARTTEST",Qt::CaseSensitive)){
    if(QString(data).contains("@@STARTTEST",Qt::CaseSensitive)){
        qDebug() <<"auto test";
        autoTest();
    }

}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), serial->errorString());
        closeSerialPort();
    }
}

void MainWindow::initActionsConnections()
{
    connect(ui->actionRefresh,&QAction::triggered,this,&MainWindow::reFresh);
    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::manOpenSerialPort);
    connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::manCloseSerialPort);
    connect(ui->actionConfigure, &QAction::triggered, settings, &MainWindow::show);
    connect(ui->actionClear, &QAction::triggered, console, &Console::clear);
    connect(ui->actionClose, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionPlayAudio,&QAction::triggered,this,&MainWindow::playAudio);
    connect(ui->actionSentroot,&QAction::triggered,this,&MainWindow::sentRoot);
    connect(ui->actionEnter,&QAction::triggered,this,&MainWindow::enter);
    connect(ui->actionSerial,&QAction::triggered,this,&MainWindow::serialwigte);
    connect(ui->actionSetSlaveTime,&QAction::triggered,this,&MainWindow::setSlaveTime);
    connect(ui->actionHunterTimeUpdate,&QAction::triggered,this,&MainWindow::HTUpdateTime);
    connect(ui->actionHunterStoreTimeCheck,&QAction::triggered,this,&MainWindow::HTCheckStoreTime);
    connect(ui->actionAutoTest,&QAction::triggered,this,&MainWindow::autoTest);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
}

void MainWindow::showStatusMessage(const QString &message)
{
    status->setText(message);
}

void MainWindow::serialwigte()
{
//    QMessageBox::critical(this,"SerialWigte","This Action will call serialWigte class");
    system("/usr/bin/hardwaretest_serial &");
    showStatusMessage(tr("open serial deamon OK!!"));
}

void MainWindow::setSlaveTime()
{
    QString data = "date -s \""+QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")+"\"\n";
    serial->write(data.toLocal8Bit());
    QByteArray data2 = "hwclock -w\n";
    serial->write(data2);
    showStatusMessage(tr("setTime!!"));
}

void MainWindow::reFresh()
{
    closeSerialPort();
    openSerialPort();
    showStatusMessage(tr("Testing Device is ") + testDevice + ", reFresh OK!!");
    //qDebug() << "Refresh";
}

void MainWindow::playAudio()
{
    autoRefreshTimer->stop();
    system("gst-play-1.0 /usr/hardwaretest/AudioTest.wav");
    showStatusMessage(tr("Playing audio ... ..."));
    autoRefreshTimer->start(5000);
}

void MainWindow::sentRoot()
{
    QByteArray data = "root";
    serial->write(data);
    showStatusMessage(tr("send root command OK!!"));
}

void MainWindow::enter()
{
    QByteArray data = "\n";
    serial->write(data);
    showStatusMessage(tr("send enter command OK!!"));
}

void MainWindow::HTUpdateTime()
{
    QByteArray data = "ubuntu\n";
    serial->write(data);
    Delay_MSec(5000);
    serial->write(data);
    data = "sleep 5 && sudo hwclock -w\n";
    serial->write(data);
    data = "\n\n\n\n";
    serial->write(data);
    data = "sudo hwclock\n";
    serial->write(data);
    showStatusMessage(tr("Update time ok, check the time!!"));
}

void MainWindow::HTCheckStoreTime()
{
    QByteArray data = "ubuntu\n";
    serial->write(data);
    Delay_MSec(5000);
    serial->write(data);
    data = "\n\n\n\n";
    serial->write(data);
    data = "sudo getpowerofftime\n";
    serial->write(data);
    showStatusMessage(tr("check the time!!"));
}

void MainWindow::autoTest()
{
    autoRefreshTimer->stop();

    QProgressDialog *progressDialog=new QProgressDialog(this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setMinimumDuration(0); //display progressDialog quickly, default is 4 seconds.
    progressDialog->setWindowTitle(tr("Wait"));
    progressDialog->setLabelText(tr("Testing..."));
    progressDialog->setCancelButton(0); //disable cancel button
    //progressDialog->setCancelButtonText(tr("Cancel"));
    progressDialog->setRange(0,100);
    progressDialog->setValue(10);

    //add this to display progressDialog quickly
    QEventLoop eventloop;
    QTimer::singleShot(1000, &eventloop,SLOT(quit()));
    eventloop.exec();

//    connect(&thread, &MasterThread::request, this,&MainWindow::showRequest);
//    thread.startMaster();
    showStatusMessage(tr("Testing serial... ..."));
    // CAN Init
    system("echo >/tmp/can0.txt");
    system("canconfig can0 stop");
    if (testDevice == "RK3568" || testDevice == "RK3588" || testDevice == "IMX8MP" || testDevice == "STMP25")  {
        system("canconfig can0 bitrate 100000 ctrlmode triple-sampling on loopback off ");
    } else {
        system("canconfig can0 bitrate 10000 ctrlmode triple-sampling on loopback off ");
    }
    system("canconfig can0 start");
    system("cansend can0 0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88");
    system("candump can0 > /tmp/can0.txt &");

    system("echo >/tmp/can1.txt");
    system("canconfig can1 stop");
    if (testDevice == "RK3568" || testDevice == "RK3588" || testDevice == "IMX8MP" || testDevice == "STMP25") {
        system("canconfig can1 bitrate 100000 ctrlmode triple-sampling on loopback off ");
    } else {
        system("canconfig can1 bitrate 10000 ctrlmode triple-sampling on loopback off ");
    }
    system("canconfig can1 start");
    system("cansend can1 0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88");
    system("candump can1 > /tmp/can1.txt &");

    // Serail Init And Test
    QString requestData = "abcdefjhijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-+=,.?/;:'!@#$%^&*()";
    QSerialPort serial;
    QByteArray responseData;
    QString testResult = "";
    testResult += "***************************\n";
    testResult += "Device: " + testDevice+"\n";
    testResult += "***************************\n";
    testResult += "Serial Port:\n";
    QString cmdstr = "echo > /tmp/serial.txt";
    system(cmdstr.toLocal8Bit());
    int testTimes = 0;

    while(testTimes < 8){

        foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts()){
            serial.setPortName(info.portName());
            serial.setBaudRate(QSerialPort::Baud115200);
            serial.setDataBits(QSerialPort::Data8);
            serial.setParity(QSerialPort::NoParity);
            serial.setStopBits(QSerialPort::OneStop);
            serial.setFlowControl(QSerialPort::NoFlowControl);

            if(serial.isOpen() || MainWindow::serial->isOpen())
            {
                serial.close();
                MainWindow::serial->close();
            }
            if(serial.open(QIODevice::ReadWrite)){

                // write request
                serial.write(requestData.toLocal8Bit());
                if(serial.waitForBytesWritten(500)){

                    // read response
                    if(serial.waitForReadyRead(1000)){
                        responseData = serial.readAll();
                        while(serial.waitForReadyRead(10))
                            responseData += serial.readAll();
                        QString response(responseData);
                        if (requestData == response) {
                                //testResult +=QString(info.portName()) + " is OK.\n";
                                cmdstr="echo " + QString(info.portName()) +" OK." + " >> /tmp/serial.txt";
                                system(cmdstr.toLocal8Bit());
                        }
                        else {
                            //testResult +=QString(info.portName()) + " is Not OK.\n";
                            cmdstr="echo " + QString(info.portName()) +" NOK." + " >> /tmp/serial.txt";
                            system(cmdstr.toLocal8Bit());
                        }
                    } else {
                        //testResult +=QString(info.portName()) + " is Not Detected.\n";
                        cmdstr="echo " + QString(info.portName()) +" NOD." + " >> /tmp/serial.txt";
                        system(cmdstr.toLocal8Bit());
                    }
                } else
                    showStatusMessage(QString(info.portName())+" write error.");
            } else
                    showStatusMessage(QString(info.portName())+" open error");
        }
        testTimes++;
        progressDialog->setValue(testTimes*10+10);
    }

    //QEventLoop eventloop;
    QTimer::singleShot(1000, &eventloop,SLOT(quit()));
    eventloop.exec();

    QString filePath0 = "/tmp/serial.txt";
    QFile file0(filePath0);
    if (file0.open(QIODevice::ReadWrite)){
        QTextStream in(&file0);
        QString line=in.readAll();
        //ttymxc0
        if (line.contains("ttymxc0 OK")){
            testResult += "COM0 is OK.\n";
        } else if(line.contains("ttymxc0 NOD")){
            testResult += "COM0 is Not Detected.\n";
        } else {
            testResult += "COM0 is Not OK.\n";
        }

        //ttymxc1
        if (line.contains("ttymxc1 OK")){
            if(testDevice == "CS10600RA070") {
                testResult += "COM2 is OK.\n";
            }else{
                testResult += "COM1 is OK.\n";
            }
        } else if(line.contains("ttymxc1 NOD")){
            if(testDevice == "CS10600RA070") {
                testResult += "COM2 is Not Detected.\n";
            }else{
                if(testDevice != "CS10600RA4070" && testDevice != "CS10600RA4070D" && testDevice != "CS12720RA4050" && testDevice != "CS12800RA101" && testDevice != "CS12800RA4101BOX" && testDevice != "CS12800RA4101" && testDevice != "LRRA4-101" && testDevice != "CS12800RA4101A" && testDevice != "CS12800RA4101P" && testDevice != "CS19108RA4133P" && testDevice != "CS10768RA4150P" && testDevice != "CS19108RA4156P" && testDevice != "CS19108RA4215P" && testDevice !="AM335XBOARD" && testDevice !="CS12800PX101" && testDevice !="CS12800R101P" && testDevice !="RK3568" && testDevice !="RK3588" && testDevice !="IMX8MP" && testDevice != "CS10600RA5070P" && testDevice != "CS19108RA4133PISO"){
                    testResult += "COM1 is Not Detected.\n";
                }else if(testDevice == "RK3568" || testDevice == "RK3588" || testDevice == "IMX8MP"){
                    testResult += "COM1 is Not Detected. RK3568 RK3588 IMX8MP 5 and 10 Ignore it\n";
                }
            }
        } else {
            if(testDevice == "CS10600RA070") {
                testResult += "COM2 is Not OK.\n";
            }else{
                testResult += "COM1 is Not OK.\n";
            }
        }

        //ttymxc2
        if (line.contains("ttymxc2 OK")){
            if(testDevice == "CS10600RA070" || testDevice =="CS10600RA4070" || testDevice =="CS10600RA4070D" || testDevice == "CS12720RA4050" || testDevice == "CS12800RA4101BOX" || testDevice =="CS12800RA101" || testDevice == "CS12800RA4101" || testDevice == "LRRA4-101" || testDevice == "CS12800RA4101A" || testDevice == "CS12800RA4101P" || testDevice == "CS19108RA4133P" || testDevice == "CS10768RA4150P" || testDevice == "CS19108RA4156P" || testDevice == "CS19108RA4215P" || testDevice =="AM335XBOARD" || testDevice =="CS12800R101P" || testDevice =="CS10600RA5070P" || testDevice == "CS19108RA4133PISO") {
                testResult += "COM1 is OK.\n";
            }else{
                testResult += "COM2 is OK.\n";
            }
        } else if(line.contains("ttymxc2 NOD")){
            if(testDevice == "CS10600RA070" || testDevice =="CS10600RA4070" || testDevice =="CS10600RA4070D" || testDevice == "CS12720RA4050" || testDevice == "CS12800RA4101BOX" || testDevice =="CS12800RA101"  || testDevice == "CS12800RA4101" || testDevice == "LRRA4-101" || testDevice == "CS12800RA4101A" || testDevice == "CS12800RA4101P" || testDevice == "CS19108RA4133P" || testDevice == "CS10768RA4150P" || testDevice == "CS19108RA4156P" || testDevice == "CS19108RA4215P" || testDevice =="AM335XBOARD" || testDevice =="CS12800R101P" || testDevice =="CS10600RA5070P" || testDevice == "CS19108RA4133PISO") {
                testResult += "COM1 is Not Detected.\n";
            }else if(testDevice =="CS12800PX101"){

            }else{
                testResult += "COM2 is Not Detected.\n";
            }
        } else {
            if(testDevice == "CS10600RA070" || testDevice =="CS10600RA4070" || testDevice =="CS10600RA4070D" || testDevice == "CS12720RA4050" || testDevice =="CS12800RA4101BOX" || testDevice =="CS12800RA101"  || testDevice == "CS12800RA4101" || testDevice == "LRRA4-101" || testDevice == "CS12800RA4101A" || testDevice == "CS12800RA4101P" || testDevice == "CS19108RA4133P" || testDevice == "CS10768RA4150P" || testDevice == "CS19108RA4156P" || testDevice == "CS19108RA4215P" || testDevice =="AM335XBOARD" || testDevice =="CS12800R101P" || testDevice =="CS10600RA5070P" || testDevice == "CS19108RA4133PISO") {
                testResult += "COM1 is Not OK.\n";
            }else{
                testResult += "COM2 is Not OK.\n";
            }
        }

        //ttymxc3
        if (line.contains("ttymxc3 OK")){
            if(testDevice == "CS10600RA4070" || testDevice == "CS10600RA4070D" || testDevice == "CS12720RA4050" || testDevice =="CS12800RA4101BOX" || testDevice == "CS12800RA4101" || testDevice == "LRRA4-101" || testDevice == "CS12800RA4101A" || testDevice == "CS12800RA4101P" || testDevice == "CS19108RA4133P" || testDevice == "CS10768RA4150P" || testDevice == "CS19108RA4156P" || testDevice == "CS19108RA4215P" || testDevice =="AM335XBOARD" || testDevice =="CS12800R101P" || testDevice =="CS10600RA5070P" || testDevice == "CS19108RA4133PISO"){
                testResult += "COM2 is OK.\n";
            }else if(testDevice == "CS12800RA101"){
                testResult += "COM4 is OK.\n";
            }else if(testDevice =="CS12800PX101"){
                testResult += "COM1 is OK.\n";
            }else{
                testResult += "COM3 is OK.\n";
            }
        }else if(line.contains("ttymxc3 NOD")){
            if(testDevice != "CS10600RA070"){
                if(testDevice == "CS10600RA4070" || testDevice == "CS10600RA4070D" || testDevice == "CS12720RA4050" || testDevice == "CS12800RA4101BOX" || testDevice == "CS12800RA4101" || testDevice == "LRRA4-101" || testDevice == "CS12800RA4101A" || testDevice == "CS12800RA4101P" || testDevice == "CS19108RA4133P" || testDevice == "CS10768RA4150P" || testDevice == "CS19108RA4156P" || testDevice == "CS19108RA4215P" || testDevice == "AM335XBOARD" || testDevice =="CS12800R101P" || testDevice =="CS10600RA5070P" || testDevice == "CS19108RA4133PISO"){
                    testResult += "COM2 is Not Detected.\n";
                }else if(testDevice == "CS12800RA101"){
                    testResult += "COM4 is Not Detected.\n";
                }else if(testDevice == "CS12800PX101"){
                    testResult += "COM1 is Not Detected.\n";
                }else{
                    testResult += "COM3 is Not Detected.\n";
                }
            }
        } else {
            if(testDevice == "CS10600RA4070" || testDevice == "CS10600RA4070D" || testDevice == "CS12720RA4050" || testDevice == "CS12800RA4101BOX" || testDevice == "CS12800RA4101" || testDevice == "LRRA4-101" || testDevice == "CS12800RA4101A" || testDevice == "CS12800RA4101P" || testDevice == "CS19108RA4133P" || testDevice == "CS10768RA4150P" || testDevice == "CS19108RA4156P" || testDevice == "CS19108RA4215P" || testDevice == "AM335XBOARD" || testDevice =="CS12800R101P" || testDevice =="CS10600RA5070P" || testDevice == "CS19108RA4133PISO"){
                testResult += "COM2 is Not OK.\n";
            }else if(testDevice == "CS12800RA101"){
                testResult += "COM4 is Not OK.\n";
            }else if(testDevice == "CS12800PX101"){
                testResult += "COM1 is Not OK.\n";
            }else{
                testResult += "COM3 is Not OK.\n";
            }
        }

        //ttymxc4
        if (line.contains("ttymxc4 OK")){
            if(testDevice == "CS10600RA4070" || testDevice == "CS10600RA4070D" || testDevice == "CS12720RA4050" || testDevice == "CS12800RA4101BOX" || testDevice == "CS12800RA4101P" || testDevice == "CS19108RA4133P" || testDevice == "CS10768RA4150P" || testDevice == "CS19108RA4156P" || testDevice == "CS19108RA4215P" || testDevice =="AM335XBOARD" || testDevice =="CS12800R101P" || testDevice =="CS10600RA5070P"){
                testResult += "COM3 is OK.\n";
            }else if(testDevice == "CS12800RA101"){
                testResult += "COM5 is OK.\n";
            }else{
                testResult += "COM4 is OK.\n";
            }
        } else if(line.contains("ttymxc4 NOD")){
            if(testDevice != "CS10600RA070" && testDevice != "CS12800RA4101" && testDevice != "LRRA4-101" && testDevice != "CS12800RA4101A"){
                if(testDevice == "CS10600RA4070" || testDevice == "CS10600RA4070D" || testDevice == "CS12720RA4050" || testDevice == "CS12800RA4101BOX" || testDevice == "CS12800RA4101P" || testDevice == "CS19108RA4133P" || testDevice == "CS10768RA4150P" || testDevice == "CS19108RA4156P" || testDevice == "CS19108RA4215P" || testDevice =="AM335XBOARD" || testDevice =="CS12800R101P" || testDevice =="CS10600RA5070P"){
                    testResult += "COM3 is Not Detected.\n";
                }else if(testDevice == "CS12800RA101"){
                    testResult += "COM5 is Not Detected.\n";
                }else if(testDevice =="CS12800PX101" || testDevice == "CS19108RA4133PISO"){
                }else{
                    testResult += "COM4 is Not Detected.\n";
                }
            }
        } else {
            if(testDevice =="CS10600RA4070" || testDevice =="CS10600RA4070D" || testDevice == "CS12720RA4050" || testDevice == "CS12800RA4101BOX" || testDevice == "CS12800RA4101P" || testDevice == "CS19108RA4133P" || testDevice == "CS10768RA4150P" || testDevice == "CS19108RA4156P" || testDevice == "CS19108RA4215P" || testDevice =="AM335XBOARD" || testDevice =="CS12800R101P" || testDevice =="CS10600RA5070P"){
                testResult += "COM3 is Not OK.\n";
            }else if(testDevice == "CS12800RA101"){
                testResult += "COM5 is Not OK.\n";
            }else{
                testResult += "COM4 is Not OK.\n";
            }
        }

        // recheck ttymxc0 which will break other serial port.
        // one bug for this application
        if(testResult.contains("COM0 is Not Detected")){
            testResult = "COM0 error, fix and test again.\n";
        }

    } else {
        showStatusMessage(QString("Cann't open file %1, error code is %2").arg(filePath0).arg(file0.error()));
    }
    file0.close();

    // CS12800RA101 ttyUSB0/ttyUSB1
    if(testDevice == "CS12800RA101"){
        QString filePath1 = "/tmp/serial23.txt";
        QFile file1(filePath1);
        if (file1.open(QIODevice::ReadWrite)){
            QTextStream in(&file1);
            QString line=in.readAll();

            if(line.contains("ttyUSB0 OK")){
                testResult +="COM2 is OK.\n";
            } else if(line.contains("ttyUSB0 NOK")){
                testResult +="COM2 is Not OK.\n";
            } else{
                testResult +="COM2 is Not Detected.\n";
            }

            if(line.contains("ttyUSB1 OK")){
                testResult +="COM3 is OK.\n";
            } else if(line.contains("ttyUSB1 NOK")){
                testResult +="COM3 is Not OK.\n";
            } else{
                testResult +="COM3 is Not Detected.\n";
            }

        } else {
            showStatusMessage(QString("Cann't open file %1, error code is %2").arg(filePath1).arg(file1.error()));
        }
        file1.close();
    }

    // CS12800R101P ttyS2
    if(testDevice == "CS12800R101P"){
        QString filePath1 = "/tmp/ttys2.txt";
        QFile file1(filePath1);
        if (file1.open(QIODevice::ReadWrite)){
            QTextStream in(&file1);
            QString line=in.readAll();

            if(line.contains("ttyS2 OK")){
                testResult +="ttyS2 is OK.\n";
            } else if(line.contains("ttyS2 NOK")){
                testResult +="ttyS2 is Not OK.\n";
            } else{
                testResult +="ttyS2 is Not Detected.\n";
            }

        } else {
            showStatusMessage(QString("Cann't open file %1, error code is %2").arg(filePath1).arg(file1.error()));
        }
        file1.close();
    }

    if(testDevice != "CS10600RA070" && testDevice != "CS12800RA4101" && testDevice != "LRRA4-101" && testDevice != "CS12800RA4101A"){
        testResult += "\n";
        testResult += "CAN Port:\n";
    }

    // Test CAN
    //QEventLoop eventloop;
    QTimer::singleShot(1000, &eventloop,SLOT(quit()));
    eventloop.exec();

    showStatusMessage(tr("Testing can... ..."));
    QString can0filePath = "/tmp/can0.txt";
    QFile can0file(can0filePath);
    if (can0file.open(QIODevice::ReadWrite)){
        QTextStream in(&can0file);
        QString line=in.readAll();
        if(line.contains("11 22 33 44 55 66 77 88")){
            testResult += "CAN0 is OK.\n";
        } else {
            if(testDevice != "CS10600RA070" && testDevice != "CS12800RA4101" && testDevice != "LRRA4-101" && testDevice != "CS12800RA4101A" && testDevice !="RK3568" && testDevice != "RK3588" && testDevice != "IMX8MP"){
                testResult += "CAN0 is not OK.\n";
            } else if (testDevice == "RK3568" || testDevice == "RK3588" || testDevice == "IMX8MP") {
                testResult += "CAN0 is not OK. RK3568 RK3588 IMX8MP 5 Ignore it.\n";
            }
        }
    } else {
        showStatusMessage(QString("Cann't open file %1, error code is %2").arg(can0filePath).arg(can0file.error()));
    }
    can0file.close();

    QString can1filePath = "/tmp/can1.txt";
    QFile can1file(can1filePath);
    if (can1file.open(QIODevice::ReadWrite)){
        QTextStream in(&can1file);
        QString line=in.readAll();
        if(line.contains("11 22 33 44 55 66 77 88")){
            testResult += "CAN1 is OK.\n";
        } else {
            if(testDevice != "CS10600RA4070" && testDevice != "CS10600RA4070D" && testDevice != "CS12720RA4050" && testDevice != "CS12800RA4101BOX" && testDevice != "CS12800RA101" && testDevice != "CS10600RA070" && testDevice != "CS12800RA4101" && testDevice != "LRRA4-101" && testDevice != "CS12800RA4101A" && testDevice != "CS12800RA4101P" && testDevice != "CS19108RA4133P" && testDevice != "CS10768RA4150P" && testDevice != "CS19108RA4156P" && testDevice != "CS19108RA4215P" && testDevice !="CS12800PX101" && testDevice !="CS12800R101P" && testDevice != "CS10600RA5070P" && testDevice != "CS19108RA4133PISO")
            {
                testResult += "CAN1 is not OK.\n";
            }
        }
    } else {
        showStatusMessage(QString("Cann't open file %1, error code is %2").arg(can1filePath).arg(can1file.error()));
    }
    can1file.close();

    // Get RTC and Zigbee result
    testResult += "\n";
    testResult += "Other device:\n";

    QString rtcfilePath = "/tmp/rtc.txt";
        QFile rtcfile(rtcfilePath);
        if (rtcfile.open(QIODevice::ReadWrite)){
            QTextStream in(&rtcfile);
            QString line=in.readAll();
            testResult += line;
        } else {
            showStatusMessage(QString("Cann't open file %1, error code is %2").arg(rtcfilePath).arg(rtcfile.error()));
        }
        rtcfile.close();

    QString zigbeefilePath = "/tmp/zigbee.txt";
    QFile zigbeefile(zigbeefilePath);
    if (zigbeefile.open(QIODevice::ReadWrite)){
        QTextStream in(&zigbeefile);
        QString line=in.readAll();
        testResult += line;
    } else {
        showStatusMessage(QString("Cann't open file %1, error code is %2").arg(zigbeefilePath).arg(zigbeefile.error()));
    }
    zigbeefile.close();

    progressDialog->setValue(100);
    progressDialog->wasCanceled();
    showStatusMessage("Test end!");
    system("killall gst-play-1.0");
    system("gst-play-1.0 /usr/hardwaretest/AutoTestFinish.aac >/dev/null &");   
    showRequest(testResult);

    // clear temp file
    cmdstr = "rm /tmp/serial.txt && rm /tmp/can*.txt";
    system(cmdstr.toLocal8Bit());

    autoRefreshTimer->start(5000);
}

void MainWindow::showRequest(const QString &s)
{
    //QMessageBox::critical(this,"TestResult",s);
    //add auto close feature
    QMessageBox *box = new QMessageBox(QMessageBox::Information,"TestResult",s);
    QTimer::singleShot(20000,box,SLOT(close()));
    box->exec();
}
