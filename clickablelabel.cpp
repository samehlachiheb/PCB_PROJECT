#include "clickablelabel.h"
#include<QFrame>
ClickableLabel::ClickableLabel(QWidget* parent)
    : QFrame(parent) // Change from QLabel(parent) to QFrame(parent)
{

}

void ClickableLabel::mousePressEvent(QMouseEvent* event)
{
    emit clicked();
    QFrame::mousePressEvent(event); // Change from QLabel::mousePressEvent(event) to QFrame::mousePressEvent(event)
}
