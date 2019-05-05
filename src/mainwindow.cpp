#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "confedit.h"
#include "importconf.h"
#include "vinteract.h"
#include <QHeaderView>
#include <QStandardItemModel>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QFile>
#include "db.h"
#include "vmess.h"
#include <QCloseEvent>
#include <QFileInfo>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    this->setWindowIcon(QIcon("Himeki.ico"));
    ui->setupUi(this);
    updateConfTable();
    ui->configTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->configTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showMenu(QPoint)));
    this->v2Inst = new v2Instance();
    hTray = new QSystemTrayIcon();
    hTray->setToolTip("Hv2ray");
    hTray->setIcon(QIcon("Himeki.ico"));
    connect(hTray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(on_activatedTray(QSystemTrayIcon::ActivationReason)));
    createTrayAction();
    if(QFileInfo("config.json").exists()) {
        v2Inst->start(this);
    }
    QAction *select = new QAction("Select", ui->configTable);
    QAction *del = new QAction("Delete", ui->configTable);
    QAction *rename = new QAction("Rename", ui->configTable);
    popMenu->addAction(select);
    popMenu->addAction(del);
    popMenu->addAction(rename);
    connect(select, SIGNAL(triggered()), this, SLOT(select_triggered()));
    connect(del, SIGNAL(triggered()), this, SLOT(delConf()));
    connect(rename, SIGNAL(triggered()), this, SLOT(renameRow()));
    connect(ui->logText, SIGNAL(textChanged()), this, SLOT(scrollToBottom()));
    bar = ui->logText->verticalScrollBar();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete this->v2Inst;
}

void MainWindow::on_actionEdit_triggered()
{
    this->e = new ConfEdit(this);
    e->show();
}

void MainWindow::on_actionExisting_config_triggered()
{
    importConf *f = new importConf(this);
    f->show();
}
void MainWindow::showMenu(QPoint pos)
{
    if(ui->configTable->indexAt(pos).column() != -1) {
        popMenu->move(cursor().pos());
        popMenu->show();
    }
}
void MainWindow::select_triggered()
{
    int row = ui->configTable->selectionModel()->currentIndex().row();
    int idIntable = ui->configTable->model()->data(ui->configTable->model()->index(row, 4)).toInt();
    this->geneConf(idIntable);
    if(this->v2Inst->v2Process->state() == QProcess::Running) {
        this->on_restartButton_clicked();
    }
}

void MainWindow::delConf()
{
    int row = ui->configTable->selectionModel()->currentIndex().row();
    int idIntable = ui->configTable->model()->data(ui->configTable->model()->index(row, 4)).toInt();
    QString queryString = "delete from confs where id = " + QString::number(idIntable);
    db myDb;
    myDb.query(queryString);
    QString rmFile = "conf/" + QString::number(idIntable) + ".conf";
    QFile::remove(rmFile);
    emit updateConfTable();
}
void MainWindow::updateConfTable()
{
    db myDb;
    myDb.query("select COUNT(*) from confs;");
    myDb.myQuery.first();
    int rows = myDb.myQuery.value(0).toInt();
    QStandardItemModel* model = new QStandardItemModel(rows, 5);
    ui->configTable->setModel(model);
    model->setHeaderData(0, Qt::Horizontal, "Alias");
    model->setHeaderData(1, Qt::Horizontal, "Host");
    model->setHeaderData(2, Qt::Horizontal, "Port");
    model->setHeaderData(3, Qt::Horizontal, "Checked");
    model->setHeaderData(4, Qt::Horizontal, "idInTable");
    ui->configTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->configTable->setColumnHidden(4, true);
    ui->configTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->configTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    myDb.query("select * from confs");
    myDb.myQuery.first();
    for (int i = 0; i < rows; ++i) {
        model->setItem(i, 0, new QStandardItem(myDb.myQuery.value(3).toString()));
        model->setItem(i, 1, new QStandardItem(myDb.myQuery.value(1).toString()));
        model->setItem(i, 2, new QStandardItem(myDb.myQuery.value(2).toString()));
        model->setItem(i, 4, new QStandardItem(myDb.myQuery.value(0).toString()));
        if (myDb.myQuery.value(8).toInt() == 1) {
            model->setItem(i, 3, new QStandardItem("√"));
        }
        if (i < rows - 1) {
            myDb.myQuery.next();
        }
    }
}
void MainWindow::geneConf(int idIntable)
{
    vConfig tmpConf;
    db myDb;
    myDb.query("update confs set selected = 0");
    QString queryString = "update confs set selected = 1 where id = " + QString::number(idIntable);
    myDb.query(queryString);
    emit updateConfTable();
    tmpConf.query(idIntable);
    if (tmpConf.isCustom == 1) {
        QString src = "conf/" + QString::number(idIntable) + ".conf";
        if (QFile::exists("config.json")) {
            QFile::remove("config.json");
        }
        QFile::copy(src, "config.json");
    } else { //Config generator
    }
}
void MainWindow::updateLog()
{
    ui->logText->insertPlainText(this->v2Inst->v2Process->readAllStandardOutput());
}

void MainWindow::on_startButton_clicked()
{
    ui->logText->clear();
    this->v2Inst->start(this);
}

void MainWindow::on_stopButton_clicked()
{
    this->v2Inst->stop();
    ui->logText->clear();
}

void MainWindow::on_restartButton_clicked()
{
    this->v2Inst->stop();
    ui->logText->clear();
    this->v2Inst->start(this);
}

void MainWindow::on_clbutton_clicked()
{
    ui->logText->clear();
}

void MainWindow::on_rtButton_clicked()
{
    emit updateConfTable();
}

void MainWindow::on_actionVmess_triggered()
{
    vmess *inVmess = new vmess(this);
    inVmess->show();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    this->hide();
    event->ignore();
}

void MainWindow::createTrayAction()
{
    QAction *actionShow = new QAction(this);
    QAction *actionQuit = new QAction(this);
    QAction *actionStart = new QAction(this);
    QAction *actionRestart = new QAction(this);
    QAction *actionStop = new QAction(this);
    actionShow->setText("Show main widget");
    actionQuit->setText("Quit");
    actionStart->setText("Start v2ray");
    actionRestart->setText("Restart v2ray");
    actionStop->setText("Stop v2ray");
    trayMenu->addAction(actionShow);
    trayMenu->addAction(actionStart);
    trayMenu->addAction(actionStop);
    trayMenu->addAction(actionRestart);
    trayMenu->addAction(actionQuit);
    connect(actionShow, SIGNAL(triggered()), this, SLOT(showMainWindow()));
    connect(actionStop, SIGNAL(triggered()), this, SLOT(on_stopButton_clicked()));
    connect(actionQuit, SIGNAL(triggered()), this, SLOT(quit()));
    connect(actionRestart, SIGNAL(triggered()), this, SLOT(on_restartButton_clicked()));
    connect(actionStart, SIGNAL(triggered()), this, SLOT(on_startButton_clicked()));
    hTray->setContextMenu(trayMenu);
    hTray->show();
}
void MainWindow::on_activatedTray(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::Trigger:
            if(this->isHidden()) {
                this->show();
            }
            break;
        case QSystemTrayIcon::DoubleClick:
            if(this->isHidden()) {
                this->show();
            }
            break;
        case QSystemTrayIcon::MiddleClick:
            break;
        case QSystemTrayIcon::Unknown:
            break;
    }
}
void MainWindow::showMainWindow()
{
    if(this->isHidden()) {
        this->show();
    }
}

void MainWindow::quit()
{
    QCoreApplication::quit();
}

void MainWindow::on_actionExit_triggered()
{
    QCoreApplication::quit();
}

void MainWindow::renameRow()
{
    QString text = QInputDialog::getText(this, "Rename config", "New name:", QLineEdit::Normal);
    int row = ui->configTable->currentIndex().row();
    int idIntable = ui->configTable->model()->data(ui->configTable->model()->index(row, 4)).toInt();
    db mydb;
    QString updateString = "update confs set alias = '" + text + "' where id = " + QString::number(idIntable);
    mydb.query(updateString);
    emit updateConfTable();
}
void MainWindow::scrollToBottom()
{
    bar->setValue(bar->maximum());
}
