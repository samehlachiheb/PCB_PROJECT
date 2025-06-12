// clickablelabel.h
#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QFrame>      // Ensure this is QFrame, not QLabel
#include <QWidget>
#include <QMouseEvent> // Make sure QMouseEvent is included for mousePressEvent

class ClickableLabel : public QFrame { // Ensure it publicly inherits from QFrame
    Q_OBJECT // Essential for Qt's meta-object system (signals/slots)
public:
    explicit ClickableLabel(QWidget* parent = nullptr); // Constructor

signals:
    void clicked(); // Signal emitted when the label is clicked

protected:
    // Override mousePressEvent to emit the clicked signal
    void mousePressEvent(QMouseEvent* event) override;
};

#endif // CLICKABLELABEL_H
