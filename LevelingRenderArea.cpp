#include "LevelingRenderArea.h"

#include <QPainter>
#include <QLinearGradient>
#include <iostream>
#include <stdlib.h>
#include <QMouseEvent>
#include <vector>
#include <QMutexLocker>

#define IMAGE_MARGIN 6
#define ELLIPSE_SIZE 6

using std::cout;
using std::endl;


LevelingRenderArea::LevelingRenderArea(QWidget *parent)
    : QWidget(parent)
{
    interpolator = NULL;

    connect(&thread, SIGNAL(renderedImage(QImage)), this, SLOT(updatePixmap(QImage)));

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

void LevelingRenderArea::resizeEvent(QResizeEvent *)
{
    QSize s;
    s.setWidth(size().width() - (IMAGE_MARGIN*2));
    s.setHeight(size().height() - (IMAGE_MARGIN*2));
    pixmap = QPixmap();
    thread.render(this->interpolator, s);
    this->update();
}

void LevelingRenderArea::setInterpolator(const Interpolator *interpolator)
{
    this->interpolator = interpolator;
    pixmap = QPixmap();
    QSize s;
    s.setWidth(size().width() - (IMAGE_MARGIN*2));
    s.setHeight(size().height() - (IMAGE_MARGIN*2));
    thread.render(interpolator, s);
    this->update();
}

void LevelingRenderArea::updatePixmap(const QImage &image)
{
    pixmap = QPixmap::fromImage(image);
    update();
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

    if (interpolator->getType() == Interpolator::SINGLE)
    {
        return;
    }

    if (pixmap.isNull())
    {
        painter.setPen(Qt::black);
        painter.drawText(rect(), Qt::AlignCenter, tr("Rendering image, please wait..."));
        return;
    }

    painter.drawPixmap(QPoint(IMAGE_MARGIN,IMAGE_MARGIN), pixmap);

}

double RenderThread::remap(double value, double low1, double high1, double low2, double high2)
{
    return low2 + (value - low1) * ((high2 - low2) / (high1 - low1));
}

double RenderThread::remap(double value, double high1, double high2)
{
    return value * high2 / high1;
}

RenderThread::RenderThread(QObject *parent)
    : QThread(parent)
{
    abort = false;
    restart = false;
}

RenderThread::~RenderThread()
{
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();
}

void RenderThread::render(const Interpolator *interpolator, QSize size)
{
    if (interpolator == NULL)
    {
        return;
    }

    QMutexLocker locker(&mutex);
    this->interpolator = interpolator;
    this->size = size;

    if (!isRunning()){
        start(LowPriority);
    } else {
        restart = true;
        condition.wakeOne();
    }
}

void RenderThread::run()
{
    forever{
        mutex.lock();
        QSize size = this->size;
        const Interpolator *interpolator = this->interpolator;
        mutex.unlock();

        QImage finalImage(size, QImage::Format_RGB32);
        QPainter painter(&finalImage);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(finalImage.rect(), Qt::lightGray);

        for (int i = ELLIPSE_SIZE; i <= size.width()-ELLIPSE_SIZE-1; i++)
        {
            for (int j = ELLIPSE_SIZE; j <= size.height() - ELLIPSE_SIZE-1; j++){
                if (restart) {
                    break;
                }
                if (abort)
                {
                    return;
                }

                double yVal = 0;

                //Remap the viewport xy coordinate to interpoler xy
                double remappedI = remap(double(i), ELLIPSE_SIZE, size.width() - ELLIPSE_SIZE - 1, 0, interpolator->getXValue(interpolator->getXSteps() - 1));
                double remappedJ = remap(double((size.height() - 1)-j), ELLIPSE_SIZE, size.height() - ELLIPSE_SIZE - 1, 0, interpolator->getYValue(interpolator->getYSteps() - 1));

                //Create a gradient with the three colors that will represent the height.
                QLinearGradient gr(QPointF(0,0), QPointF(1005,0));
                gr.setColorAt(0, QColor(255,0,0));

                //Get the median of all the zprobes, so the predominant color is the central one.
                //So we set the main color using the median as the position of the color in the gradient, instead of simply putting it at the center.
                double median = remap(interpolator->getMedian(), interpolator->getMinZValue(), interpolator->getMaxZValue(), 0, 1);

                gr.setColorAt(median, QColor(0, 255, 0));
                gr.setColorAt(1, QColor(0, 0, 255));

                QImage img(1006, 1, QImage::Format_RGB32);
                QPainter p(&img);
                p.fillRect(img.rect(), gr);

                //Do the interpolation
                interpolator->interpolate(remappedI, remappedJ, yVal);

                double yValRemapped = remap(yVal, interpolator->getMinZValue(), interpolator->getMaxZValue(), 5, 1005);
    //            std::cout << "Original i: " << i << " RemappedI = " << remappedI << " - Original J: "
    //                      << j << " RemappedJ = " << remappedJ << " Value: "
    //                      << yVal << " Remapped: " << yValRemapped << std::endl;
                QColor c = Qt::gray;

                //Avoid trying to get colors from the image gradient outside its size.
                //This can happen with some image zones.
                if (yValRemapped >=0 && yValRemapped < img.width())
                {
                    c = img.pixel(yValRemapped, 0);
                }

                painter.setPen(QPen(c, 1));
                painter.drawPoint(QPoint(i,j));
            }
            if (restart)
            {
                break;
            }
        }

        if (!restart){
            //Draw the test points
            painter.setPen(QPen(Qt::black, 1));
            for (unsigned int i = 0; i<interpolator->getXSteps(); i++)
            {
                for (unsigned int j = 0; j<interpolator->getYSteps(); j++)
                {
                    int x = remap(interpolator->getXValue(i), 0, interpolator->getXValue(interpolator->getXSteps() - 1), ELLIPSE_SIZE, size.width() - ELLIPSE_SIZE - 1 );
                    int y = remap(interpolator->getYValue(j), 0, interpolator->getYValue(interpolator->getYSteps() - 1), ELLIPSE_SIZE, size.height() - ELLIPSE_SIZE - 1);
                    painter.drawEllipse(QPoint(x,y),ELLIPSE_SIZE,ELLIPSE_SIZE);
                }
            }

            //Draw the grid lines.
            painter.setPen(QPen(Qt::black, 1, Qt::DashLine));

            for (unsigned int i = 0; i<interpolator->getXSteps(); i++)
            {
                int x = remap(interpolator->getXValue(i), 0, interpolator->getXValue(interpolator->getXSteps() - 1), ELLIPSE_SIZE, size.width() - ELLIPSE_SIZE - 1);
                painter.drawLine(QPoint(x, 0), QPoint(x, size.height()-1));

            }
            for (unsigned int j = 0; j<interpolator->getYSteps(); j++)
            {
                int y = remap(interpolator->getYValue(j), 0, interpolator->getYValue(interpolator->getYSteps() - 1), ELLIPSE_SIZE, size.height() - ELLIPSE_SIZE - 1);
                painter.drawLine(QPoint(0, y), QPoint(size.width()-1, y));

            }

            emit renderedImage(finalImage);
        }


        //Wait for the next image.
        mutex.lock();
        if (!restart)
        {
            condition.wait(&mutex);
        }
        restart = false;
        mutex.unlock();

    }
}
