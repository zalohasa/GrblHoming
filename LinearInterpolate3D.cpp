#include "LinearInterpolate3D.h"
#include <iostream>
#include <string.h>

using std::cout;
using std::endl;

LinearInterpolate3D::LinearInterpolate3D(const double * xValues, unsigned int xCount, const double * yValues, unsigned int yCount, const double * xyValues, double offset)
{

    nValuesX = xCount;
    nValuesY = yCount;
    initialOffset = offset;

    this->xValues = new double[nValuesX];
    this->yValues = new double[nValuesY];
    this->xyValues = new double[nValuesX * nValuesY];

    memcpy(this->xValues, xValues, nValuesX*sizeof(double));
    memcpy(this->yValues, yValues, nValuesY*sizeof(double));
    memcpy(this->xyValues, xyValues, nValuesX*nValuesY*sizeof(double));

    zMin = xyValues[0];
    zMax = xyValues[0];
    median = 0;

    for (unsigned int i = 0; i<(nValuesX * nValuesY); i++)
    {
        if (xyValues[i] < zMin) zMin = xyValues[i];
        if (xyValues[i] > zMax) zMax = xyValues[i];
        median += xyValues[i];
    }

    median /= nValuesX*nValuesY;
}

LinearInterpolate3D::LinearInterpolate3D(const Interpolator *interpolator)
{
    if (interpolator == NULL)
    {
        return;
    }

    nValuesX = interpolator->getXSteps();
    nValuesY = interpolator->getYSteps();
    initialOffset = interpolator->getInitialOffset();

    this->xValues = new double[nValuesX];
    this->yValues = new double[nValuesY];
    this->xyValues = new double[nValuesX * nValuesY];

    memcpy(this->xValues, interpolator->getXValues(), nValuesX*sizeof(double));
    memcpy(this->yValues, interpolator->getYValues(), nValuesY*sizeof(double));
    memcpy(this->xyValues, interpolator->getXYValues(), nValuesX*nValuesY*sizeof(double));

    zMin = interpolator->getMinZValue();
    zMax = interpolator->getMaxZValue();
    median = interpolator->getMedian();
}

LinearInterpolate3D::~LinearInterpolate3D()
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

bool LinearInterpolate3D::findCoeficents(const double * values, unsigned int nValues, double x, unsigned int & a, unsigned int & b) const
{

    //Check if the x is out of the area.
    if (x < values[0]) {
        a = 0;
        return true; //Exact match
    }

    if (x > values[nValues-1]){
        a = nValues - 1;
        return true; //Exact match
    }

    //If the x is inside the area, find the coeficents.
    for (int j = 0; j < nValues; j++)
    {
        //search the coeficents.
        if (values[j] == x)
        {
            a = j; //We need to know the column to interpolate Y coordinate.
            return true; //exact match
        } else if (x > values[j] && x < values[j+1] ) {
            a = j;
            b = j+1;
            break;
        }
    }
    return false;
}

bool LinearInterpolate3D::interpolate(double x, double y, double & res) const
{
    unsigned int ax = 0, bx = 0;
    unsigned int ay = 0, by = 0;

    //Let's find the x coeficents:
    bool exactMatchX = findCoeficents(xValues, nValuesX, x, ax, bx);
    bool exactMatchY = findCoeficents(yValues, nValuesY, y, ay, by);

    double y0, y1;

    if (exactMatchX && exactMatchY)
    {
        res = xyValues[ay*nValuesX + ax];
        return true;
    } else if (exactMatchX){
        y0 = xyValues[ay*nValuesX + ax];
        y1 = xyValues[by*nValuesX + ax];
        res = lerp(y0, y1, normaliceValue(yValues[ay], yValues[by], y));
    } else if (exactMatchY){
        y0 = xyValues[ay*nValuesX + ax];
        y1 = xyValues[ay*nValuesX + bx];
        res = lerp(y0, y1, normaliceValue(xValues[ax], xValues[bx], x));
    } else {
        double yr0, yr1;
        //First column
        y0 = xyValues[ay*nValuesX + ax];
        y1 = xyValues[by*nValuesX + ax];
        yr0 = lerp(y0, y1, normaliceValue(yValues[ay], yValues[by], y));

        //Second column
        y0 = xyValues[ay*nValuesX + bx];
        y1 = xyValues[by*nValuesX + bx];
        yr1 = lerp(y0, y1, normaliceValue(yValues[ay], yValues[by], y));

        res = lerp(yr0, yr1, normaliceValue(xValues[ax], xValues[bx], x));

    }
    return false;
}

double LinearInterpolate3D::lerp(const double y0, const double y1,const double x) const {
    return (1-x)*y0 + x*y1;
}



