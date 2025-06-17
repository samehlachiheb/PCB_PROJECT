#ifndef DRAWINGWINDOW_H
#define DRAWINGWINDOW_H

#include <QMainWindow>
#include "imageviewer.h"
#include <opencv2/opencv.hpp>
#include <QPushButton>

class DrawingWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit DrawingWindow(QWidget *parent = nullptr);
    ~DrawingWindow();

    void setOriginalImage(const cv::Mat& image);
    cv::Mat getResultImage() const;

private slots:
    void onSaveContours();
    void onUndoLastContour(); // ADDED: Slot for undo button
    void onClearAllContours(); // ADDED: Slot for clear all button

private:
    ImageViewer *imageViewer;
    QPushButton *saveButton;
    QPushButton *undoButton; // ADDED: Undo button
    QPushButton *clearAllButton; // ADDED: Clear All button
};

#endif // DRAWINGWINDOW_H
