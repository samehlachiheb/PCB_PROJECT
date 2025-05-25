// clickablelabel.cpp
#include "clickablelabel.h"

ClickableLabel::ClickableLabel(QWidget* parent)
    : QLabel(parent)
{

}

void ClickableLabel::mousePressEvent(QMouseEvent* event)
{
    emit clicked();
    QLabel::mousePressEvent(event);
}
