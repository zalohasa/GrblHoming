#ifndef SPILINEINTERPOLATE_H
#define SPILINEINTERPOLATE_H

#include "interpolator.h"

class SpilineInterpolate3D : public Interpolator
{
public:
    SpilineInterpolate3D(const double * xValues, unsigned int xCount, const double * yValues, unsigned int yCount, const double * xyValues, double offset);
    SpilineInterpolate3D(const Interpolator* interpolator);

    bool interpolate(double x, double y, double & res) const;
    interpolator_t getType() const { return SPILINE; }

    virtual ~SpilineInterpolate3D();

private:
    double cubicInterpolate(const double p[], double x) const;
    bool findCoeficents(const double * values, unsigned int nValues, double x, unsigned int & a, unsigned int & b, unsigned int & c, unsigned int & d) const;
};
#endif // SPILINEINTERPOLATE_H
