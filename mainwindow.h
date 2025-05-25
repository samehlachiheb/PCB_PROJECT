#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <opencv2/opencv.hpp>
#include "imagewindow.h"
#include"QMessageBox"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void loadImageFromFile();
    void updateSliderValue1(int value);
    void updateSliderValue2(int value);
    void updateSliderValue3(int value);
   void updateContoursView();


private:
    Ui::MainWindow *ui;
    ImageWindow *maskWindow = nullptr;
    ImageWindow *resultWindow = nullptr;

    cv::Mat image;
    cv::Mat maskImage;
    cv::Mat contoursImage;
protected:
   void resizeEvent(QResizeEvent *event) override;
public:
    void afficherMessage(QWidget *parent, const QString &texte,
                                       const QString &titre, QMessageBox::Icon style, int duree);

};

#endif // MAINWINDOW_H

