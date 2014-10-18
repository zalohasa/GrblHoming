#include "SingleInterpolate.h"
#include <iostream>
#include <string.h>

/*
 * Mock interpolator to support simple tool height auto adjust.
 *
 * Gonzalo López 2014
 *
 */

using std::cout;
using std::endl;

SingleInterpolate::SingleInterpolate(const double xValue, const double yValue, const double xyValue, double offset)
{

    nValuesX = 1;
    nValuesY = 1;
    initialOffset = offset;

    this->xValues = new double[nValuesX];
    this->yValues = new double[nValuesY];
    this->xyValues = new double[nValuesX * nValuesY];

    this->xValues[0] = xValue;
    this->yValues[0] = yValue;
    this->xyValues[0] = xyValue;

    zMin = xyValues[0];
    zMax = xyValues[0];
    median = xyValues[0];

}

SingleInterpolate::~SingleInterpolate()
{
    if (this->xValues != NULL)
    {
        delete [] this->xValues;
        this->xValues = NULL;
    }

    if (this->yValues != NULL)
    {
        delete [] this->yValues;
        this->yValues = NULL;
    }

    if (this->xyValues != NULL)
    {
        delete [] this->xyValues;
        this->xyValues = NULL;
    }
}

bool SingleInterpolate::interpolate(double x, double y, double & res) const
{
    res = xyValues[0];
    return true;

}





