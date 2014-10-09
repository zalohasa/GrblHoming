#include "LevelingRenderArea.h"

#include <QPainter>
#include <QLinearGradient>
#include <iostream>
#include <stdlib.h>
#include <QMouseEvent>
#include <vector>

using std::cout;
using std::endl;


LevelingRenderArea::LevelingRenderArea(QWidget *parent)
    : QWidget(parent)
{
    interpolator = NULL;
    xSteps = 5;
    ySteps = 5;
    lastWidth = 0;
    lastHeight = 0;
    forceReload = false;
    xValues = NULL;
    yValues = NULL;
    xyValues = NULL;
    //recalculatePoints(100, 100);



}

QSize LevelingRenderArea::minimumSizeHint() const
{
    return QSize(100, 100);
}

QSize LevelingRenderArea::sizeHint() const
{
    return QSize(1000, 1000);
}

//void LevelingRenderArea::mousePressEvent(QMouseEvent * mouseEvent)
//{
//    if (mouseEvent->button() == Qt::LeftButton)
//    {
//        forceReload = true;
//        this->update();
//    }
//}

void LevelingRenderArea::setInterpolator(SpilineInterpolate3D *interpolator)
{
    this->interpolator = interpolator;
    this->update();
}

void LevelingRenderArea::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QRect viewport = painter.viewport();

    painter.fillRect(viewport, Qt::lightGray);

    if (this->interpolator == NULL){
        return;
    }

    std::cout << "Minimal Z: " << interpolator->getMinZValue() << " - Maximal Z: " << interpolator->getMaxZValue() << std::endl;

    for (int i = 6; i <= viewport.right()-6; i++)
    {
        for (int j = 6; j <= viewport.bottom()-6; j++){

            double yVal = 0;

            //Remap the viewport xy coordinate to interpoler xy
            double remappedI = remap(double(i), 6, viewport.right()-6, 0, interpolator->getXValue(interpolator->getXSteps() - 1));
            double remappedJ = remap(double(viewport.bottom()-j), 6, viewport.bottom()-6, 0, interpolator->getYValue(interpolator->getYSteps() - 1));

            //Create a gradient with the three colors that will represent the height.
            QLinearGradient gr(QPointF(0,0), QPointF(1003,0));
            gr.setColorAt(0, QColor(255,0,0));

            //Get the median of all the zprobes, so the predominant color is the central one.
            //So we set the main color using the median as the position of the color in the gradient, instead of simply putting it at the center.
            double median = remap(interpolator->getMedian(), interpolator->getMinZValue(), interpolator->getMaxZValue(), 0, 1);

            gr.setColorAt(median, QColor(0, 255, 0));
            gr.setColorAt(1, QColor(0, 0, 255));

            QImage img(1003, 1, QImage::Format_RGB32);
            QPainter p(&img);
            p.fillRect(img.rect(), gr);

            //Do the interpolation
            interpolator->bicubicInterpolate(remappedI, remappedJ, yVal);

            double yValRemapped = remap(yVal, interpolator->getMinZValue(), interpolator->getMaxZValue(), 3, 1000);
//            std::cout << "Original i: " << i << " RemappedI = " << remappedI << " - Original J: "
//                      << j << " RemappedJ = " << remappedJ << " Value: "
//                      << yVal << " Remapped: " << yValRemapped << std::endl;

            QColor c = img.pixel(yValRemapped, 0);

            painter.setPen(QPen(c, 1));
            painter.drawPoint(QPoint(i,j));
        }
    }

    //Draw the test points
    painter.setPen(QPen(Qt::black, 1));
    for (int i = 0; i<interpolator->getXSteps(); i++)
    {
        for (int j = 0; j<interpolator->getYSteps(); j++)
        {
            int x = remap(interpolator->getXValue(i), 0, interpolator->getXValue(interpolator->getXSteps() - 1), 6, viewport.right() - 6);
            int y = remap(interpolator->getYValue(j), 0, interpolator->getYValue(interpolator->getYSteps() - 1), 6, viewport.bottom() - 6);
            painter.drawEllipse(QPoint(x,y),6,6);
        }
    }

    //Draw the grid lines.
    painter.setPen(QPen(Qt::black, 1, Qt::DashLine));

    for (int i = 0; i<interpolator->getXSteps(); i++)
    {
        int x = remap(interpolator->getXValue(i), 0, interpolator->getXValue(interpolator->getXSteps() - 1), 6, viewport.right() - 6);
        painter.drawLine(QPoint(x, 0), QPoint(x, viewport.bottom()));

    }
    for (int j = 0; j<interpolator->getYSteps(); j++)
    {
        int y = remap(interpolator->getYValue(j), 0, interpolator->getYValue(interpolator->getYSteps() - 1), 6, viewport.bottom() - 6);
        painter.drawLine(QPoint(0, y), QPoint(viewport.right(), y));

    }

}

double LevelingRenderArea::remap(double value, double low1, double high1, double low2, double high2)
{
    return low2 + (value - low1) * ((high2 - low2) / (high1 - low1));
}

double LevelingRenderArea::remap(double value, double high1, double high2)
{
    return value * high2 / high1;
}
