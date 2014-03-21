#ifndef SPILINEINTERPOLATE_H
#define SPILINEINTERPOLATE_H

class SpilineInterpolate3D
{
public:
    SpilineInterpolate3D(const double * xValues, unsigned int xCount, const double * yValues, unsigned int yCount, const double * xyValues);

    bool bicubicInterpolate(double x, double y, double & res);

    virtual ~SpilineInterpolate3D();

protected:
    static double cubicInterpolate(const double p[], double x);
    bool findCoeficents(const double * values, unsigned int nValues, double x, unsigned int & a, unsigned int & b, unsigned int & c, unsigned int & d);
    double normaliceValue(double min, double max, double value);
    double * xValues;
    double * yValues;
    double *  xyValues;
    unsigned int nValuesX;
    unsigned int nValuesY;
};

#endif // SPILINEINTERPOLATE_H
