#ifndef SPILINEINTERPOLATE_H
#define SPILINEINTERPOLATE_H

class SpilineInterpolate3D
{
public:
    SpilineInterpolate3D(const double * xValues, unsigned int xCount, const double * yValues, unsigned int yCount, const double * xyValues);

    bool bicubicInterpolate(double x, double y, double & res);

    virtual ~SpilineInterpolate3D();

    unsigned int getXSteps() const{ return nValuesX; }
    unsigned int getYSteps() const{ return nValuesY; }

    double getXValue(int index) const;
    double getYValue(int index) const;

    double getMaxZValue() const{ return zMax; }
    double getMinZValue() const{ return zMin; }

    double getMedian() const{ return median; }


protected:
    static double cubicInterpolate(const double p[], double x);
    bool findCoeficents(const double * values, unsigned int nValues, double x, unsigned int & a, unsigned int & b, unsigned int & c, unsigned int & d);
    double normaliceValue(double min, double max, double value);
    double * xValues;
    double * yValues;
    double *  xyValues;
    double zMin;
    double zMax;
    double median;
    unsigned int nValuesX;
    unsigned int nValuesY;
};

#endif // SPILINEINTERPOLATE_H
