#include "interpolator.h"

double Interpolator::normaliceValue(double min, double max, double value) const
{
    return (value - min) / (max - min);
}

double Interpolator::getXValue(int index) const
{
    if (index < nValuesX)
    {
        return xValues[index];
    }
    return 0;
}

double Interpolator::getYValue(int index) const
{
    if (index < nValuesY)
    {
        return yValues[index];
    }
    return 0;
}

double Interpolator::calculateOffset(double newZValue)
{
    double initialValue = xyValues[0]; //Original Z value at (0,0);
    return initialValue - newZValue + initialOffset;
}

double Interpolator::yGridSize()
{
    if (nValuesY > 1)
        return yValues[1] - yValues[0];
    return 0;
}

double Interpolator::xGridSize()
{
    if (nValuesX > 1)
        return xValues[1] - xValues[0];
    return 0;
}
