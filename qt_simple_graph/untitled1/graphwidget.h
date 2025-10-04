#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QWidget>


class graphwidget : public QWidget
{
    Q_OBJECT
public:
    explicit graphwidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // GRAPHWIDGET_H
