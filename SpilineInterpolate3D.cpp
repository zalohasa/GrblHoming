#include "SpilineInterpolate3D.h"
#include <iostream>
#include <string.h>

using std::cout;
using std::endl;

SpilineInterpolate3D::SpilineInterpolate3D(const double * xValues, unsigned int xCount, const double * yValues, unsigned int yCount, const double * xyValues)
{

    nValuesX = xCount;
    nValuesY = yCount;

    this->xValues = new double[nValuesX];
    this->yValues = new double[nValuesY];
    this->xyValues = new double[nValuesX * nValuesY];

    memcpy(this->xValues, xValues, nValuesX*sizeof(double));
    memcpy(this->yValues, yValues, nValuesY*sizeof(double));
    memcpy(this->xyValues, xyValues, nValuesX*nValuesY*sizeof(double));

    zMin = xyValues[0];
    zMax = xyValues[0];

    for (int i = 0; i<(nValuesX * nValuesY); i++)
    {
        if (xyValues[i] < zMin) zMin = xyValues[i];
        if (xyValues[i] > zMax) zMax = xyValues[i];
    }
}

double SpilineInterpolate3D::getXValue(int index) const
{
    return xValues[index];
}

double SpilineInterpolate3D::getYValue(int index) const
{
    return yValues[index];
}

SpilineInterpolate3D::~SpilineInterpolate3D()
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

bool SpilineInterpolate3D::findCoeficents(const double * values, unsigned int nValues, double x, unsigned int & a, unsigned int & b, unsigned int & c, unsigned int & d)
{
    for (int j = 0; j < nValues; j++)
    {
        //search the coeficents.
        if (values[j] == x)
        {
            b = j; //We need to know the column to interpolate Y coordinate.
            return true; //exact match
        } else if (x > values[j] && x < values[j+1] ) {
            if (j == 0)
            {
                a = 0;
                b = j;
                c = j+1;
                d = j+2;
                break;
            } else if (j ==  nValues - 2)
            {
                a = j-1;
                b = j;
                c = j+1;
                d = j+1;
                break;
            } else {
                a = j-1;
                b = j;
                c = j+1;
                d = j+2;
                break;
            }
        }
    }
    return false;

}

double SpilineInterpolate3D::normaliceValue(double min, double max, double value)
{
    return (value - min) / (max - min);
}

bool SpilineInterpolate3D::bicubicInterpolate(double x, double y, double & res)
{
    //cout << "Interpolating value for: " << x << endl;
    unsigned int ax = 0, bx = 0, cx = 0, dx = 0;
    unsigned int ay = 0, by = 0, cy = 0, dy = 0;

    //Let's find the x coeficents:
    bool exactMatchX = findCoeficents(xValues, nValuesX, x, ax, bx, cx, dx);
    bool exactMatchY = findCoeficents(yValues, nValuesY, y, ay, by, cy, dy);

    double p[4];

    if (exactMatchX && exactMatchY)
    {
        res = xyValues[by*nValuesX + bx];
        return true;
    } else if (exactMatchX){
        p[0] = xyValues[ay*nValuesX + bx];
        p[1] = xyValues[by*nValuesX + bx];
        p[2] = xyValues[cy*nValuesX + bx];
        p[3] = xyValues[dy*nValuesX + bx];
        res = cubicInterpolate(p, normaliceValue(yValues[by], yValues[cy], y));
    } else if (exactMatchY){
        p[0] = xyValues[by*nValuesX + ax];
        p[1] = xyValues[by*nValuesX + bx];
        p[2] = xyValues[by*nValuesX + cx];
        p[3] = xyValues[by*nValuesX + dx];
        res = cubicInterpolate(p, normaliceValue(xValues[bx], xValues[cx], x));
    } else {
        double aux[4];
        //First column
        p[0] = xyValues[ay*nValuesX + ax];
        p[1] = xyValues[by*nValuesX + ax];
        p[2] = xyValues[cy*nValuesX + ax];
        p[3] = xyValues[dy*nValuesX + ax];
        aux[0] = cubicInterpolate(p, normaliceValue(yValues[by], yValues[cy], y));

        //Second column
        p[0] = xyValues[ay*nValuesX + bx];
        p[1] = xyValues[by*nValuesX + bx];
        p[2] = xyValues[cy*nValuesX + bx];
        p[3] = xyValues[dy*nValuesX + bx];
        aux[1] = cubicInterpolate(p, normaliceValue(yValues[by], yValues[cy], y));

        //Third column
        p[0] = xyValues[ay*nValuesX + cx];
        p[1] = xyValues[by*nValuesX + cx];
        p[2] = xyValues[cy*nValuesX + cx];
        p[3] = xyValues[dy*nValuesX + cx];
        aux[2] = cubicInterpolate(p, normaliceValue(yValues[by], yValues[cy], y));

        //Forth column
        p[0] = xyValues[ay*nValuesX + dx];
        p[1] = xyValues[by*nValuesX + dx];
        p[2] = xyValues[cy*nValuesX + dx];
        p[3] = xyValues[dy*nValuesX + dx];
        aux[3] = cubicInterpolate(p, normaliceValue(yValues[by], yValues[cy], y));

        res = cubicInterpolate(aux, normaliceValue(xValues[bx], xValues[cx], x));

    }
    return false;
}

double SpilineInterpolate3D::cubicInterpolate (const double p[], double x) {
    return p[1] + 0.5 * x*(p[2] - p[0] + x*(2.0*p[0] - 5.0*p[1] + 4.0*p[2] - p[3] + x*(3.0*(p[1] - p[2]) + p[3] - p[0])));
}


