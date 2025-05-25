#include "imagewindow.h"
#include "ui_imagewindow.h"
#include <QImage>
#include <QPixmap>
#include <QMessageBox>
#include <QScreen>

ImageWindow::ImageWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ImageWindow)
{
    ui->setupUi(this);
}

ImageWindow::~ImageWindow()
{
    delete ui;
}
//fonction pour applique le traitement
void ImageWindow::setOriginalImage(const cv::Mat &img)
{
    originalImage = img.clone();
    updateImage();
}
//lier la fonction avec les sliders
//1-ksize
void ImageWindow::setKsize(int value)
{
    ksize = value;
    updateImage();
}
//2-seuil bas
void ImageWindow::setLowThreshold(int value)
{
    lowThreshold = value;
    updateImage();
}
//3-seuil haut
void ImageWindow::setHighThreshold(int value)
{
    highThreshold = value;
    updateImage();
}

//1
//fonction pour applique le masque sur l'a carte'image de la carte
void ImageWindow::setmaskImage(const cv::Mat &img){
    if (img.empty()) {
        QMessageBox::warning(this, "Erreur", "L'image est vide : aucune image chargée.");
        return;
    }

    cv::Mat hsv, greenMask, compColorMask;
    cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, lowerGreen, upperGreen, greenMask);
    cv::bitwise_not(greenMask, compColorMask);
    cv::morphologyEx(compColorMask, compColorMask, cv::MORPH_OPEN,
                     cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));

    cv::Mat imgComp;
    img.copyTo(imgComp, compColorMask);

    cv::cvtColor(imgComp, preprocessedGray, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(preprocessedGray, preprocessedGray);

    QImage imgQ(preprocessedGray.data, preprocessedGray.cols, preprocessedGray.rows,
                preprocessedGray.step, QImage::Format_Grayscale8);
    ui->label->setPixmap(QPixmap::fromImage(imgQ).scaled(ui->label->size(), Qt::KeepAspectRatio));
}
//2
//Fonction de détection de contours
void ImageWindow::updateImage()
{
    if (originalImage.empty()) return;
    int k = ksize % 2 == 0 ? ksize + 1 : ksize;

    cv::Mat hsv, greenMask, compColorMask;
    cv::cvtColor(originalImage, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, lowerGreen, upperGreen, greenMask);
    cv::bitwise_not(greenMask, compColorMask);
    cv::morphologyEx(compColorMask, compColorMask, cv::MORPH_OPEN,
                     cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));

    cv::Mat imgComp;
    originalImage.copyTo(imgComp, compColorMask);

    cv::Mat gray, blurred;
    cv::cvtColor(imgComp, gray, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(gray, gray);
    //cv::GaussianBlur(gray, blurred, cv::Size(ksize, ksize), 5);

    cv::Mat edges;
    cv::Canny(gray, edges, lowThreshold, highThreshold);
    cv::dilate(edges, edges, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7)));

    cv::Mat dist;
    cv::distanceTransform(edges, dist, cv::DIST_L2, 3);
    cv::normalize(dist, dist, 0, 1.0, cv::NORM_MINMAX);
    cv::Mat markersBin;
    cv::threshold(dist, markersBin, 0.4, 1.0, cv::THRESH_BINARY);
    markersBin.convertTo(markersBin, CV_8U);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(markersBin, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    cv::Mat markersFinal = cv::Mat::zeros(markersBin.size(), CV_32S);
    for (size_t i = 0; i < contours.size(); ++i)
        drawContours(markersFinal, contours, static_cast<int>(i), cv::Scalar(static_cast<int>(i) + 1), -1);

    cv::watershed(originalImage, markersFinal);

    cv::Mat display;
    cv::cvtColor(originalImage, display, cv::COLOR_BGR2GRAY);
    cv::cvtColor(display, display, cv::COLOR_GRAY2BGR);

    for (int y = 0; y < markersFinal.rows; ++y) {
        for (int x = 0; x < markersFinal.cols; ++x) {
            if (markersFinal.at<int>(y, x) == -1)
                display.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 255, 0);
        }
    }

    QImage qimg(display.data, display.cols, display.rows, display.step, QImage::Format_BGR888);
    ui->label->setPixmap(QPixmap::fromImage(qimg).scaled(ui->label->size(), Qt::KeepAspectRatio));

    displayResult = display.clone();
}




//get les images à partir d' imagewindow
cv::Mat ImageWindow::getPreprocessedGray() const {
    return preprocessedGray.clone();
}

cv::Mat ImageWindow::getContoursImage() const {
    return displayResult.clone();
}


void ImageWindow::setImage(const QPixmap &pixmap) {
    if (!pixmap.isNull()) {
        QPixmap scaledPixmap = pixmap.scaled(ui->label->size(),
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation);
        ui->label->setPixmap(scaledPixmap);
        ui->label->setAlignment(Qt::AlignCenter);
    }
}

//

void ImageWindow::showRawImage(const cv::Mat &img)
{
    if (img.empty()) return;

    cv::Mat rgb;
    if (img.channels() == 1) {
        cv::cvtColor(img, rgb, cv::COLOR_GRAY2RGB);
    } else {
        cv::cvtColor(img, rgb, cv::COLOR_BGR2RGB);
    }

    QImage qimg(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(qimg);
    ui->label->setPixmap(pixmap.scaled(ui->label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

