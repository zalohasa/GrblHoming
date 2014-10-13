#ifndef LEVELING_RENDERAREA_H
#define LEVELING_RENDERAREA_H

#include "interpolator.h"

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
    QSize sizeHint() const;

public slots:
    void setInterpolator(const Interpolator *interpolator);

protected:
    void paintEvent(QPaintEvent * event);
//    void mousePressEvent(QMouseEvent * mouseEvent);
    const Interpolator * interpolator;
    double * xValues;
    double * yValues;
    double * xyValues;
    unsigned int xSteps;
    unsigned int ySteps;
    unsigned int lastWidth;
    unsigned int lastHeight;
    bool forceReload;

private:

    /**
     * @brief remap Remaps a value in the range from low1 to high1 to a range from low2 to high2
     * @param value Value to be remapped
     * @param low1 Low end of the original range
     * @param high1 High end of the original range
     * @param low2 Low end of the target range
     * @param high2 High end of the target range.
     * @return remapped value.
     */
    double remap(double value, double low1, double high1, double low2, double high2);

    /**
     * @brief remap Remaps a value in the range from 0 to hight1 to a range from 0 to high2
     * @param value Value to be remapped
     * @param high1 High end of the original range.
     * @param high2 High end of the target range.
     * @return
     */
    double remap(double value, double high1, double high2);
};

#endif // LEVELING_RENDERAREA_H
