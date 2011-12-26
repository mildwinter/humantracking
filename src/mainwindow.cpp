#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QValidator>
#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>
#include <QDialog>
#include <QThreadPool>
#include <QMutex>
#include <QProgressBar>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    curFrame(0),
    totalFrame(0),
    captureFrame(false),
    captureFrames(false),
    hasVideo(false)
{
   // this->_timer = new QTimer(this);
    ui->setupUi(this);
    ui->txtThresh->setValidator(new QIntValidator(0, 100, this));
    ui->txtThresh->setFixedWidth(30);
    toggleDetectHuman(ui->checkHuman->isChecked());
    toggleEdge(ui->checkEdge->isChecked());
    toggleFlip(ui->checkFlip->isChecked());

    //Connect all the element
    connect(ui->checkFlip, SIGNAL(toggled(bool)), this, SLOT(toggleFlip(bool)));
    connect(ui->checkEdge, SIGNAL(toggled(bool)), this, SLOT(toggleEdge(bool)));
    connect(ui->checkHuman, SIGNAL(toggled(bool)), this, SLOT(toggleDetectHuman(bool)));
 //   connect(ui->checkEdgeInvert, SIGNAL(toggled(bool)), this, SLOT());
    connect(ui->txtThresh, SIGNAL(textChanged(QString)), this, SLOT(setThresh(QString)));
    connect(ui->slideThresh, SIGNAL(sliderMoved(int)), this, SLOT(setThresh(int)));
    //connect(this->_timer, SIGNAL(timeout()), this, SLOT(timerTick()));
    connect(ui->btnBrowse, SIGNAL(clicked()), this, SLOT(loadFile()));
    connect(ui->slideTimeline, SIGNAL(sliderMoved(int)), this, SLOT(setTimeline(int)));
    connect(ui->btnSnap, SIGNAL(clicked()), this, SLOT(toggleCaptureFrame()));
    connect(ui->btnPlayPause, SIGNAL(clicked()), this, SLOT(togglePlayPause()));

    //Check the effect setting

    this->exports = new Exports(this);
    this->settings = new Settings(this);
    this->aboutUs = new About(this);
    this->effect = new Effects();

    aboutUs->setFixedSize(400, 180);
    //set of action
    connect(ui->actionOpen_File, SIGNAL(triggered()), this, SLOT(loadFile()));
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionExports, SIGNAL(triggered()), exports, SLOT(open()));
    connect(ui->actionGlobal_Settings, SIGNAL(triggered()), settings, SLOT(open()));
    connect(ui->actionAbout_Us, SIGNAL(triggered()), aboutUs, SLOT(open()));
    connect(ui->actionCapture_all_frames, SIGNAL(triggered()), this, SLOT(toggleCaptureFrames()));


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::toggleDetectHuman(bool state){

    if(state){
        ui->groupAlgorithm->show();
        if(ui->radioSurf->isChecked()) toggleSurf(true);
        if(ui->radioHog->isChecked()) toggleHog(true);
    } else {
        ui->groupAlgorithm->hide();
        toggleSurf(false);
        toggleHog(false);
    }
}


void MainWindow::toggleEdge(bool state){
    if(state) ui->groupThresh->show();
    else ui->groupThresh->hide();
}

void MainWindow::toggleFlip(bool state){
    if(state) ui->groupFlip->show();
    else ui->groupFlip->hide();
}

void MainWindow::togglePlayPause(){
    if(!this->mThread->pause){
        this->mThread->pause = true;
        ui->btnPlayPause->setIcon(QIcon(":/images/play"));
    } else {
        mThread->pause = false;
        QThreadPool::globalInstance()->start(new MyTask(this->mThread));
        ui->btnPlayPause->setIcon(QIcon(":/images/pause"));
    }
//    if(this->_timer->isActive()){
//        this->_timer->stop();
//        ui->btnPlayPause->setIcon(QIcon(":/images/play"));
//    } else {
//        this->_timer->start(this->settings->getVideoFrame());
//        ui->btnPlayPause->setIcon(QIcon(":/images/pause"));
//    }
}

void MainWindow::toggleCaptureFrame(){
    this->captureFrame = true;
}

void MainWindow::toggleCaptureFrames(){

//    if(!this->_timer->isActive())
//        this->_timer->start(0);

    this->curFrame = 0;
    this->capture->set(CV_CAP_PROP_POS_FRAMES, 0);
    this->captureFrames = true;

    QDateTime timestem = QDateTime::currentDateTime();
    //this->settings->getSnapPath();
    this->folderPath = this->settings->getSnapPath() + "/" + (timestem.toString("yy-MM-dd hh-mm-ss"));

    QDir dir(this->folderPath);

    if(!dir.exists(this->folderPath)){
        dir.mkdir(this->folderPath);
    }

    this->dialogSnaps = new dialogSnapFrames(this, this->totalFrame);
    this->dialogSnaps->setFixedWidth(300);
    this->dialogSnaps->open();
}

void MainWindow::setThresh(int val){
    ui->txtThresh->setText(QString::number(val));
}

void MainWindow::setThresh(QString val){
    ui->slideThresh->setValue(val.toInt());
}

void MainWindow::setTimeline(int val){
    //mThread->pauseAt = val;
    emit setCurPos(val);
    curFrame = val;
}

void MainWindow::saveToFolder(Mat &img){
    QDateTime timestem = QDateTime::currentDateTime();
    string path = this->settings->getSnapPath().toStdString() + "/" + (timestem.toString("yy-MM-dd hh-mm-ss")).toStdString() + ".jpg";
    cv::imwrite(path, img);
    QMessageBox msgBox;
    msgBox.setText("The frame has been saved.");
    msgBox.setWindowTitle("Information");
    msgBox.exec();
    this->captureFrame = false;
}

void MainWindow::snapAllFrames(Mat &img){

    string fileName = this->folderPath.toStdString() + "/" + QString::number(this->curFrame).toStdString() + ".jpg";
    cv::imwrite(fileName, img);

    emit displayCurProgress(this->curFrame, this->totalFrame);
}

//void timerTick2(){

//    cv::Mat img;

//    this->capture.read(img);

//    //this->capture >> img;
//    if(img.data){

//            //Check if capture all frames is set and cur frame is at initial 0 position
//            if(this->captureFrames && this->curFrame == 0){
//                 this->capture.read(img);
//                 ui->labelDisplay->setPixmap(QPixmap::fromImage(QImage(img.data,img.cols, img.rows, img.step, QImage::Format_RGB888)).scaled(ui->labelDisplay->size(), Qt::KeepAspectRatio));
//                 ui->slideTimeline->setValue(0);
//                 ui->labelTimeline->setText(QString::number(this->curFrame) + "/" + QString::number(this->totalFrame) + "\n" + QString::number((int)this->curFrame / 30) + "/" + QString::number((int)this->totalFrame/30));
//            }

//            //Check Human Detect state
//            if(ui->checkHuman->isChecked() && ui->radioSurf->isChecked()){
//                effect->SurfD(img);
//            } else if(ui->checkHuman->isChecked() && ui->radioHog->isChecked()){
//                effect->HogD(img);
//            }

//            //Check Edge Detection state
//            if(ui->checkEdge->isChecked() && ui->checkEdgeInvert->isChecked()){
//                effect->Edge(img, img, ui->slideThresh->value(), true);
//            } else if(ui->checkEdge->isChecked()){
//                effect->Edge(img, img, ui->slideThresh->value(), false);
//            }

//            //Check Flip state
//            if(ui->checkFlip->isChecked() && ui->checkFlipHor->isChecked() && ui->checkFlipVer->isChecked()){
//                effect->Flip(img, img, -1); //Both
//            } else if(ui->checkFlip->isChecked() && ui->checkFlipVer->isChecked()){
//                effect->Flip(img, img, 0); //Ver
//            } else if(ui->checkFlip->isChecked()  && ui->checkFlipHor->isChecked()){
//                effect->Flip(img, img, 1); //Hor
//            }

//            //Check Capture Frame State
//            if(this->captureFrame){
//                this->saveToFolder(img);
//            }

//            //capture all frame if captureframes is set and curframe % 30 = 0
//            if(this->captureFrames){
//                this->snapAllFrames(img);
//                for(int i = 0; i < this->settings->getFrameToSkip(); i++) this->capture.read(img);
//                this->curFrame += this->settings->getFrameToSkip();
//                //this->capture.set(CV_CAP_PROP_POS_FRAMES, (double)this->curFrame);
//            }

//            //If capture all is set, freeze the display label
//            if(!this->captureFrames){
//                ui->labelDisplay->setPixmap(QPixmap::fromImage(QImage(img.data,img.cols, img.rows, img.step, QImage::Format_RGB888)).scaled(ui->labelDisplay->size(), Qt::KeepAspectRatio));
//                ui->slideTimeline->setValue(curFrame);
//                ui->labelTimeline->setText(QString::number(this->curFrame) + "/" + QString::number(this->totalFrame) + "\n" + QString::number((int)this->curFrame / 30) + "/" + QString::number((int)this->totalFrame/30));
//                ++curFrame; //Next frame

//            }

//    } else {
//        if(this->captureFrames){
//            this->captureFrames = false;
//            this->capture.set(CV_CAP_PROP_POS_FRAMES, 1);
//            this->curFrame = 0;
//        }
//        if(this->dialogSnaps->isVisible())
//            this->dialogSnaps->close();
//    }

//}

//Connect all the effect button and display that want to be send to thread
void MainWindow::initEffectAndGui(){
    //To passing object Mat via signal and slot
    typedef Mat AMAT;
    qRegisterMetaType<AMAT>("Mat");

    connect(this->mThread, SIGNAL(currentFrame(int,Mat)), this, SLOT(displayResult(int,Mat)));
    connect(ui->radioSurf, SIGNAL(toggled(bool)), this, SLOT(toggleSurf(bool)));
    connect(ui->radioHog, SIGNAL(toggled(bool)), this, SLOT(toggleHog(bool)));

}

void MainWindow::loadFile(){
//    this->_timer->stop();
    //Check if the thread already run
//    if(mThread->isRunning()){
//        delete mThread;
//        //mThread = 0;
//    }

    this->curFrame = 0;
    const QString path = QFileDialog::getOpenFileName(this, "Select files", "", "Video Files (*.avi)");
    if(!path.isEmpty()){

        //Check if the thread already run
        QMutex mutex;
        mutex.lock();
        if(this->hasVideo){
            //this->mThread->stop = true;
            //this->mThread->destroy();
            //delete this->mThread;
            disconnect(this->mThread, SIGNAL(currentFrame(int,Mat)), this, SLOT(displayResult(int,Mat)));
            disconnect(ui->radioSurf, SIGNAL(toggled(bool)), this, SLOT(toggleSurf(bool)));
            disconnect(ui->radioHog, SIGNAL(toggled(bool)), this, SLOT(toggleHog(bool)));
            QThreadPool::globalInstance()->releaseThread();
            delete this->mThread;
            delete this->capture;
            //this->mThread = 0;
            //this->capture = 0;
        }
        mutex.unlock();


        this->capture = new cv::VideoCapture(path.toStdString());
        this->mThread = new processThread(this, this->capture, "");

        //Connect all the effect button
        this->initEffectAndGui();

        this->totalFrame = (int)this->capture->get(CV_CAP_PROP_FRAME_COUNT);
        ui->slideTimeline->setMaximum(this->totalFrame);
     //   this->_timer->start(this->settings->getVideoFrame());
        ui->btnPlayPause->setIcon(QIcon(":/images/pause"));

        this->setInitialProp();
        //this->mThread->start();

        this->hasVideo = true;
        //mThread->stop = false;

        QThreadPool::globalInstance()->start(new MyTask(this->mThread));
    }
}

void MainWindow::setInitialProp(){
    //Check Human Detection
    if(ui->checkHuman->isChecked() && ui->radioSurf->isChecked()){
        //effect->SurfD(img);
        this->mThread->surf = true;
    } else if(ui->checkHuman->isChecked() && ui->radioHog->isChecked()){
       // effect->HogD(img);
        this->mThread->hog = true;
    }

    //Check Edge Detection state
    if(ui->checkEdge->isChecked() && ui->checkEdgeInvert->isChecked()){
        //effect->Edge(img, img, ui->slideThresh->value(), true);
        this->mThread->edge = true; this->mThread->edgeInvert = true; this->mThread->edgeThresh = ui->slideThresh->value();
    } else if(ui->checkEdge->isChecked()){
        //effect->Edge(img, img, ui->slideThresh->value(), false);
        this->mThread->edge = true; this->mThread->edgeInvert = true;
    }

    //Check Flip state
    if(ui->checkFlip->isChecked() && ui->checkFlipHor->isChecked() && ui->checkFlipVer->isChecked()){
        //effect->Flip(img, img, -1); //Both
        this->mThread->flip = true; this->mThread->flipCode = -1;
    } else if(ui->checkFlip->isChecked() && ui->checkFlipVer->isChecked()){
        //effect->Flip(img, img, 0); //Ver
        this->mThread->flip = true; this->mThread->flipCode = 0;
    } else if(ui->checkFlip->isChecked()  && ui->checkFlipHor->isChecked()){
        //effect->Flip(img, img, 1); //Hor
        this->mThread->flip = true; this->mThread->flipCode = 1;
    }

    this->mThread->fps = this->settings->getVideoFrame();

}

void MainWindow::displayResult(int cur, Mat img)
{
    ui->labelDisplay->setPixmap(QPixmap::fromImage(QImage(img.data,img.cols, img.rows, img.step, QImage::Format_RGB888)).scaled(ui->labelDisplay->size(), Qt::KeepAspectRatio));
    ui->slideTimeline->setValue(cur);
    ui->labelTimeline->setText(QString::number(cur) + "/" + QString::number(this->totalFrame) + "\n" + QString::number(cur / 30) + "/" + QString::number((int)this->totalFrame/30));
}

void MainWindow::toggleSurf(bool state)
{
    if(state && ui->checkHuman->isChecked())
        mThread->surf = true;
    else
        mThread->surf = false;
}

void MainWindow::toggleHog(bool state)
{
    if(state && ui->checkHuman->isChecked())
        mThread->hog = true;
    else
        mThread->hog = false;
}