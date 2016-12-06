#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    :QMainWindow(parent)
{
    QPushButton *pushButton = new QPushButton;
    pushButton->setText("memory leak");
    this->setCentralWidget(pushButton);
}

MainWindow::~MainWindow(){

}
