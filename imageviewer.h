#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QMouseEvent>
#include <opencv2/opencv.hpp>
#include <vector>
#include <stack> // ADDED: For undo history

class ImageViewer : public QWidget {
    Q_OBJECT

public:
    explicit ImageViewer(QWidget *parent = nullptr);
    void setImage(const cv::Mat& image);
    cv::Mat getImageWithRectangles() const;
    const std::vector<cv::Rect>& getDrawnRectangles() const { return drawn_rectangles; }

    // ADDED: Public slot for undo
public slots:
    void undoLastRectangle();
    void clearAllRectangles(); // ADDED: To clear all drawn rectangles

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    cv::Mat original_image_cv;
    QImage current_image_qt;
    std::vector<cv::Rect> drawn_rectangles;
    QPoint start_point;
    bool drawing;

    // ADDED: Stack to store history of rectangle states for undo
    std::stack<std::vector<cv::Rect>> undo_history;

    // Helper to convert cv::Mat to QImage
    QImage cvMatToQImage(const cv::Mat &mat);

    // ADDED: Helper to save current state to undo history
    void saveStateToUndoHistory();
};

#endif // IMAGEVIEWER_H
