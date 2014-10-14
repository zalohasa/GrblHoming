#ifndef LEVELING_RENDERAREA_H
#define LEVELING_RENDERAREA_H

#include "interpolator.h"

#include <QBrush>
#include <QPen>
#include <QPixmap>
#include <QWidget>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>

//! [0]

class RenderThread : public QThread
{
    Q_OBJECT
public:
    RenderThread(QObject *parent = 0);
    ~RenderThread();

    void render(const Interpolator *interpolator, QSize size);

    /**
     * @brief remap Remaps a value in the range from low1 to high1 to a range from low2 to high2
     * @param value Value to be remapped
     * @param low1 Low end of the original range
     * @param high1 High end of the original range
     * @param low2 Low end of the target range
     * @param high2 High end of the target range.
     * @return remapped value.
     */
    double static remap(double value, double low1, double high1, double low2, double high2);

    /**
     * @brief remap Remaps a value in the range from 0 to hight1 to a range from 0 to high2
     * @param value Value to be remapped
     * @param high1 High end of the original range.
     * @param high2 High end of the target range.
     * @return
     */
    double static remap(double value, double high1, double high2);

signals:
    void renderedImage(const QImage &image);
protected:
    void run();
private:
    QMutex mutex;
    QWaitCondition condition;
    const Interpolator *interpolator;
    QSize size;
    //bool restart;
    bool abort;
    bool restart;


};

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
    void resizeEvent(QResizeEvent *);

//    void mousePressEvent(QMouseEvent * mouseEvent);
    const Interpolator * interpolator;

private slots:
    void updatePixmap(const QImage &image);
private:
    RenderThread thread;
    QPixmap pixmap;

};

#endif // LEVELING_RENDERAREA_H
