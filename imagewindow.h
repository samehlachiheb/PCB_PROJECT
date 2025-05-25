#ifndef IMAGEWINDOW_H
#define IMAGEWINDOW_H

#include <QMainWindow>
#include <opencv2/opencv.hpp>

namespace Ui {
class ImageWindow;
}

class ImageWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit ImageWindow(QWidget *parent = nullptr);
    ~ImageWindow();

    void setOriginalImage(const cv::Mat &img);
    void setmaskImage(const cv::Mat &img);

    cv::Mat getPreprocessedGray() const;
    cv::Mat getContoursImage() const;

    void showRawImage(const cv::Mat& img);
public slots:
    void setKsize(int value);
    void setLowThreshold(int value);
    void setHighThreshold(int value);
    void updateImage();

private:
    Ui::ImageWindow *ui;
    cv::Mat originalImage;
    cv::Mat preprocessedGray;
    cv::Mat displayResult;

    int ksize = 5;
    int lowThreshold = 50;
    int highThreshold = 150;

    cv::Scalar lowerGreen = cv::Scalar(35, 50, 50);
    cv::Scalar upperGreen = cv::Scalar(85, 255, 255);
public:
    void setImage(const QPixmap &pixmap);

};

#endif // IMAGEWINDOW_H

