#ifndef MAINWINDOW
#define MAINWINDOW
#include <QtWidgets>

class MainWindow : public QMainWindow{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    void initData();
};

#endif // MAINWINDOW

