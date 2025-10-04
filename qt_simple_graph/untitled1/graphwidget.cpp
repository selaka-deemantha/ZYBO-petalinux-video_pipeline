#include "graphwidget.h"
#include <QPainter>

graphwidget::graphwidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(400, 300);   // Give it a reasonable default size
}

void graphwidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), Qt::black);      // Background
    p.setPen(Qt::green);
    p.drawText(10, 20, "Spectrum Graph Area");  // Placeholder text
}
