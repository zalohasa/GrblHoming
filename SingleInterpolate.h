#ifndef SINGLEINTERPOLATE_H
#define SINGLEINTERPOLATE_H

#include "interpolator.h"

class SingleInterpolate : public Interpolator
{
public:
    SingleInterpolate(const double xValue, const double yValue, const double xyValue, double offset);

    bool interpolate(double x, double y, double & res) const;

    virtual ~SingleInterpolate();

    interpolator_t getType() const { return SINGLE; }

};

#endif // SINGLEINTERPOLATE_H
