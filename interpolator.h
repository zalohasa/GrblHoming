#ifndef INTERPOLATOR_H
#define INTERPOLATOR_H

class Interpolator
{

public:

    enum interpolator_t {SPILINE = 0, LINEAR, SINGLE};
    virtual ~Interpolator() {}
    virtual bool interpolate(double x, double y, double &res) const = 0;
    virtual interpolator_t getType() const = 0;

    unsigned int getXSteps() const{ return nValuesX; }
    unsigned int getYSteps() const{ return nValuesY; }

    double getXValue(int index) const;
    double getYValue(int index) const;

    double getMaxZValue() const{ return zMax; }
    double getMinZValue() const{ return zMin; }

    double getMedian() const{ return median; }
    double getInitialOffset() const{ return initialOffset; }

    const double * getXValues() const { return xValues; }
    const double * getYValues() const { return yValues; }
    const double * getXYValues() const {return xyValues; }

    double xGridSize();
    double yGridSize();

    double calculateOffset(double newZValue);

protected:
    double normaliceValue(double min, double max, double value) const;
    double * xValues;
    double * yValues;
    double *  xyValues;
    double initialOffset;
    double zMin;
    double zMax;
    double median;
    unsigned int nValuesX;
    unsigned int nValuesY;
};

#endif // INTERPOLATOR_H
