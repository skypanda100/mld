#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    :QMainWindow(parent)
{
    this->initData();
    QPushButton *pushButton = new QPushButton;
    pushButton->setText("memory leak");
    this->setCentralWidget(pushButton);
}

MainWindow::~MainWindow(){

}

void MainWindow::initData(){
    char *new_leak = new char[503];
}
