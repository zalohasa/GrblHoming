#include "LevelingRenderArea.h"

/*
 * Leveling render area to display the Z leveling data in a visual way.
 *
 * Gonzalo López 2014
 *
 */


#include <QPainter>
#include <QLinearGradient>
#include <iostream>
#include <stdlib.h>
#include <QMouseEvent>
#include <vector>
#include <QMutexLocker>

#define IMAGE_MARGIN 6
#define ELLIPSE_SIZE 8
#define TEXT_MARGIN_Z 4

using std::cout;
using std::endl;


LevelingRenderArea::LevelingRenderArea(QWidget *parent)
    : QWidget(parent)
{
    interpolator = NULL;
    //this->setSizePolicy(QSizePolicy::Expanding);
    drawAxis = false;
    setCursor(Qt::CrossCursor);

    connect(&thread, SIGNAL(renderedImage(QImage)), this, SLOT(updatePixmap(QImage)));

}

QSize LevelingRenderArea::sizeHint() const
{
    return QSize(1000, 1000);
}

void LevelingRenderArea::resizeEvent(QResizeEvent *)
{
    QSize s;
    s.setWidth(size().width() - (IMAGE_MARGIN*2));
    s.setHeight(size().height() - (IMAGE_MARGIN*4));
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
    s.setHeight(size().height() - (IMAGE_MARGIN*4));
    thread.render(interpolator, s);
    this->update();
}

void LevelingRenderArea::updatePixmap(const QImage &image)
{
    pixmap = QPixmap::fromImage(image);
    update();
}

void LevelingRenderArea::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    setMouseTracking(true);
    drawAxis = true;
}

void LevelingRenderArea::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    setMouseTracking(false);
    drawAxis = false;
    update();
}

void LevelingRenderArea::mouseMoveEvent(QMouseEvent *event)
{
    mousePoint = event->pos();
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

    //Draw the min/max text.
    painter.drawText(QRect(IMAGE_MARGIN + ELLIPSE_SIZE, IMAGE_MARGIN+pixmap.height()- ELLIPSE_SIZE, pixmap.width() - (ELLIPSE_SIZE*2),
                           height() - (IMAGE_MARGIN+pixmap.height()- ELLIPSE_SIZE)),
                     Qt::AlignCenter,
                     QString("Min: ").append(QString::number(interpolator->getMinZValue()))
                     .append(" - Max: ").append(QString::number(interpolator->getMaxZValue()))
                     .append(" - Difference: ").append(QString::number(interpolator->getMaxZValue() - interpolator->getMinZValue())));

    //Draw the crosshair and the Z level value text.
    if (drawAxis && mousePoint.x() >= IMAGE_MARGIN + ELLIPSE_SIZE
            && mousePoint.x()<=pixmap.width()-ELLIPSE_SIZE+IMAGE_MARGIN
            && mousePoint.y() >= IMAGE_MARGIN + ELLIPSE_SIZE
            && mousePoint.y() <= pixmap.height()-ELLIPSE_SIZE+IMAGE_MARGIN)
    {
        double i = mousePoint.x() - IMAGE_MARGIN;
        double j = mousePoint.y() - IMAGE_MARGIN;

        painter.setPen(QPen(Qt::black, 1, Qt::DotLine));
        painter.drawLine(QPoint(mousePoint.x(), IMAGE_MARGIN+ELLIPSE_SIZE), QPoint(mousePoint.x(), IMAGE_MARGIN+pixmap.height()- ELLIPSE_SIZE));
        painter.drawLine(QPoint(IMAGE_MARGIN+ELLIPSE_SIZE, mousePoint.y()), QPoint(IMAGE_MARGIN + pixmap.width() - ELLIPSE_SIZE, mousePoint.y()));

        double remappedI = RenderThread::remap(double(i), ELLIPSE_SIZE, pixmap.width() - ELLIPSE_SIZE - 1, 0, interpolator->getXValue(interpolator->getXSteps() - 1));
        double remappedJ = RenderThread::remap(double((pixmap.height() - 1)-j), ELLIPSE_SIZE, pixmap.height() - ELLIPSE_SIZE - 1, 0, interpolator->getYValue(interpolator->getYSteps() - 1));

        //Do the interpolation
        double zVal;
        interpolator->interpolate(remappedI, remappedJ, zVal);

        int xText = 0, yText = 0;

        QString text = QString("Z: ").append(QString::number(zVal));
        QFontMetrics m = painter.fontMetrics();
        int textWidth = m.width(text);
        int textHeight = m.height();

        if (i > pixmap.width() - textWidth - ELLIPSE_SIZE - 5)
        {
            xText = mousePoint.x() - textWidth - TEXT_MARGIN_Z;
        }else{
            xText = mousePoint.x() + TEXT_MARGIN_Z;
        }

        if (j < textHeight + ELLIPSE_SIZE + 5)
        {
            yText = mousePoint.y() + textHeight;
        }else{
            yText = mousePoint.y() - TEXT_MARGIN_Z;
        }

        QString yDataText = QString("Y: ").append(QString::number(remappedJ));
        QString xDataText = QString("X: ").append(QString::number(remappedI));
        int yTextWidth = m.width(yDataText);

        //Z value text
        painter.drawText(QPoint(xText, yText), text);

        //X and Y value text
        if(j <= (IMAGE_MARGIN+pixmap.height()- ELLIPSE_SIZE) - textHeight - 10)
            painter.drawText(QPoint(xText, (IMAGE_MARGIN+pixmap.height()- ELLIPSE_SIZE)-TEXT_MARGIN_Z), xDataText);
        if (j > IMAGE_MARGIN + ELLIPSE_SIZE + textHeight + TEXT_MARGIN_Z + 10)
            painter.drawText(QPoint(xText, IMAGE_MARGIN + ELLIPSE_SIZE + textHeight), xDataText);
        if (i > yTextWidth + IMAGE_MARGIN + ELLIPSE_SIZE+TEXT_MARGIN_Z + 10)
            painter.drawText(QPoint(IMAGE_MARGIN + ELLIPSE_SIZE+TEXT_MARGIN_Z, yText), yDataText);
        if (i < IMAGE_MARGIN + pixmap.width() - ELLIPSE_SIZE - yTextWidth - textWidth - TEXT_MARGIN_Z - 10)
            painter.drawText(QPoint(IMAGE_MARGIN + pixmap.width() - ELLIPSE_SIZE - yTextWidth - TEXT_MARGIN_Z, yText), yDataText);

    }
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

    //Create a gradient with the five colors that will represent the height.
    QLinearGradient gr(QPointF(0,0), QPointF(1005,0));

    gr.setColorAt(0, QColor(0,0,255));
    gr.setColorAt(0.25, QColor(0, 255, 255));
    gr.setColorAt(0.5, QColor(0, 255, 0));
    gr.setColorAt(0.75, QColor(255, 255, 0));
    gr.setColorAt(1, QColor(255, 0, 0));

    //Create an image 1006 pixels width and 1 pixel height to draw the gradient.
    gradientImage = QImage(1006, 1, QImage::Format_RGB32);
    QPainter p(&gradientImage);
    p.fillRect(gradientImage.rect(), gr);
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
    if (interpolator == NULL || interpolator->getType() == Interpolator::SINGLE)
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

        for (int j = ELLIPSE_SIZE; j <= size.height()-ELLIPSE_SIZE-1; j++)
        {
            uint * line = reinterpret_cast<uint*>(finalImage.scanLine(j));
            for (int i = ELLIPSE_SIZE; i <= size.width() - ELLIPSE_SIZE-1; i++){
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

                //Do the interpolation
                interpolator->interpolate(remappedI, remappedJ, yVal);

                double yValRemapped = remap(yVal, interpolator->getMinZValue(), interpolator->getMaxZValue(), 5, 1005);
    //            std::cout << "Original i: " << i << " RemappedI = " << remappedI << " - Original J: "
    //                      << j << " RemappedJ = " << remappedJ << " Value: "
    //                      << yVal << " Remapped: " << yValRemapped << std::endl;


                //Avoid trying to get colors from the image gradient outside its size.
                //This can happen with some image zones.
                QColor c;
                if (yValRemapped<0){
                    c = QColor(0, 0, 255);
                } else if (yValRemapped > 1005)
                {
                    c = QColor(255, 0, 0);
                }
                else{
                    c = gradientImage.pixel(yValRemapped, 0);
                }

                //Paint the pixel
                line[i] = c.rgb();
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
