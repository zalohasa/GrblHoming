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
