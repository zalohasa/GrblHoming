#include "LevelingRenderArea.h"

#include <QPainter>
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

/*QSize LevelingRenderArea::sizeHint() const
{
    return QSize(400, 200);
}*/

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

void LevelingRenderArea::recalculatePoints(unsigned int width, unsigned int height)
{

    if (interpolator != NULL)
    {
        delete interpolator;
    }

    if (xValues)
        delete [] xValues;
    if (yValues)
        delete [] yValues;
    if (xyValues)
        delete [] xyValues;

    double xIncrement = width / (xSteps - 1);
    double yIncrement = height / (ySteps - 1);

    xValues = new double[xSteps];
    yValues = new double[ySteps];
    xyValues = new double[xSteps*ySteps];

    double jx = 0;
    double jy = 0;

    for (int i = 0; i<xSteps; i++)
    {

        xValues[i] = jx;
        jx+=xIncrement;

        jy = 0;
        for (int h = 0; h<ySteps; h++)
        {

            yValues[h] = jy;
            jy+=yIncrement;
            xyValues[h*xSteps + i] = rand() % 3;
        }


    }

    interpolator = new SpilineInterpolate3D(xValues, xSteps, yValues, ySteps, xyValues);


}

void LevelingRenderArea::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QRect viewport = painter.viewport();

    /*if (lastWidth != viewport.right() || lastHeight != viewport.bottom() || forceReload || interpolator == NULL)
    {
        lastWidth = viewport.right();
        lastHeight = viewport.bottom();
        forceReload = false;
        //recalculatePoints(100, 100);
    }*/

    painter.fillRect(viewport, Qt::lightGray);

    if (this->interpolator == NULL){
        return;
    }

    QPen pen(QPen(Qt::blue, 1));

    painter.setPen(pen);

    std::vector<QColor> colors;
    std::vector<QRect> points;

    std::cout << "Minimal Z: " << interpolator->getMinZValue() << " - Maximal Z: " << interpolator->getMaxZValue() << std::endl;

    for (int i = 0; i <= viewport.right(); i++)
    {
        for (int j = 0; j <= viewport.bottom(); j--){

            double yVal = 0;

            double remappedI = remap(double(i), viewport.right(), interpolator->getXValue(interpolator->getXSteps() - 1));
            double remappedJ = remap(double(j), viewport.bottom(), interpolator->getYValue(interpolator->getYSteps() - 1));



            interpolator->bicubicInterpolate(remappedI, remappedJ, yVal);

            double yValRemapped = remap(yVal, interpolator->getMinZValue(), interpolator->getMaxZValue(), 0, 255);
//            std::cout << "Original i: " << i << " RemappedI = " << remappedI << " - Original J: "
//                      << j << " RemappedJ = " << remappedJ << " Value: "
//                      << yVal << " Remapped: " << yValRemapped << std::endl;

//            if (exactMatch){
//                //painter.setPen(QPen(Qt::blue, 2));
//                //painter.setBrush(QBrush(QColor(yVal, yVal, yVal)));
//                //painter.drawRect(i-5, j-5,10,10);
//                colors.push_back(QColor(yValRemapped, yValRemapped, yValRemapped));
//                points.push_back(QRect(i-6, j-6, 12, 12));
//                //painter.setPen(QPen(QColor(yVal, yVal, yVal), 1));
//                //painter.drawPoint(QPoint(i,j));
//            } else {

                painter.setPen(QPen(QColor(yValRemapped, yValRemapped, yValRemapped), 1));
                painter.drawPoint(QPoint(i,j));
//            }


        }
    }

//   painter.setPen(QPen(Qt::blue, 2));
//    for (int i = 0; i<points.size(); i++)
//    {
//        painter.setBrush(QBrush(colors.at(i)));
//        painter.drawRect(points.at(i));
//    }


}

double LevelingRenderArea::remap(double value, double low1, double high1, double low2, double high2)
{
    return low2 + (value - low1) * ((high2 - low2) / (high1 - low1));
}

double LevelingRenderArea::remap(double value, double high1, double high2)
{
    return value * high2 / high1;
}
