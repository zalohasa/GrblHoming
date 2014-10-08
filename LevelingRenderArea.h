#ifndef LEVELING_RENDERAREA_H
#define LEVELING_RENDERAREA_H

#include "SpilineInterpolate3D.h"

#include <QBrush>
#include <QPen>
#include <QPixmap>
#include <QWidget>

//! [0]
class LevelingRenderArea : public QWidget
{
    Q_OBJECT

public:
    LevelingRenderArea(QWidget *parent = 0);

    QSize minimumSizeHint() const;
    /*QSize sizeHint() const;*/

public slots:
    void setInterpolator(SpilineInterpolate3D *interpolator);

protected:
    void recalculatePoints(unsigned int width, unsigned int height);
    void paintEvent(QPaintEvent * event);
//    void mousePressEvent(QMouseEvent * mouseEvent);
    SpilineInterpolate3D * interpolator;
    double * xValues;
    double * yValues;
    double * xyValues;
    unsigned int xSteps;
    unsigned int ySteps;
    unsigned int lastWidth;
    unsigned int lastHeight;
    bool forceReload;

private:
    double remap(double value, double low1, double high1, double low2, double high2);
    double remap(double value, double high1, double high2);
};

#endif // LEVELING_RENDERAREA_H
