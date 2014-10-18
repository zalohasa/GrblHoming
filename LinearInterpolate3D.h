#ifndef LINEARINTERPOLATE3D_H
#define LINEARINTERPOLATE3D_H

/*
 * Linear interpolator class that that supports a minimum of two probing points per axis.
 *
 * Gonzalo López 2014
 *
 */

#include "interpolator.h"

class LinearInterpolate3D : public Interpolator
{
public:
    LinearInterpolate3D(const double * xValues, unsigned int xCount, const double * yValues, unsigned int yCount, const double * xyValues, double offset);
    LinearInterpolate3D(const Interpolator* interpolator);

    bool interpolate(double x, double y, double & res) const;
    interpolator_t getType() const { return LINEAR; }

    virtual ~LinearInterpolate3D();

private:
    double lerp(const double y0, const double y1, const double x) const;
    bool findCoeficents(const double * values, unsigned int nValues, double x, unsigned int & a, unsigned int & b) const;
};

#endif // LINEARINTERPOLATE3D_H
