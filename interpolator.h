#ifndef INTERPOLATOR_H
#define INTERPOLATOR_H

/*
 * Basic interpolator class that all interpolators must derive from.
 *
 * Gonzalo López 2014
 *
 */

class Interpolator
{

public:

    enum interpolator_t {SPILINE = 0, LINEAR, SINGLE};
    virtual ~Interpolator() {}

    /**
     * @brief interpolate Perform the interpolation in the point (x,y) and places the result in res.
     * @param x X coordinate of the interpolation point
     * @param y Y coordinate of the interpolation point
     * @param res Interpolated z value for the (x,y) point.
     * @return true if the data is an exact match. (The (x,y) point is a probe point)
     */
    virtual bool interpolate(double x, double y, double &res) const = 0;

    /**
     * @brief getType Returns the type of the interpolator.
     * @return
     */
    virtual interpolator_t getType() const = 0;

    /**
     * @brief getXSteps Number of probes in the x axis.
     * @return
     */
    unsigned int getXSteps() const{ return nValuesX; }
    /**
     * @brief getYSteps Number of probes in the y axis.
     * @return
     */
    unsigned int getYSteps() const{ return nValuesY; }

    /**
     * @brief getXValue Get the X axis value of the selected point.
     * @param index Index of the axis point, from 0 to getXSteps()-1
     * @return
     */
    double getXValue(int index) const;
    /**
     * @brief getYValue Get the Y axis value of the selected point.
     * @param index Index of the axis point, from 0 to getYSteps()-1
     * @return
     */
    double getYValue(int index) const;

    /**
     * @brief getMaxZValue The max Z value in the probed data.
     * @return
     */
    double getMaxZValue() const{ return zMax; }
    /**
     * @brief getMinZValue The min Z value in the probed data.
     * @return
     */
    double getMinZValue() const{ return zMin; }

    /**
     * @brief getMedian The median of all the probed Z values.
     * @return
     */
    double getMedian() const{ return median; }

    /**
     * @brief getInitialOffset Offset used when the interpoler was first instanciated.
     * @return
     */
    double getInitialOffset() const{ return initialOffset; }

    /**
     * @brief getXValues Returns a const pointer to the X axis values array.
     * @return
     */
    const double * getXValues() const { return xValues; }

    /**
     * @brief getYValues Returns a cons pointer to the Y axis values array.
     * @return
     */
    const double * getYValues() const { return yValues; }

    /**
     * @brief getXYValues Returns a const pointer to the Z probed values.
     * The data in the array is rows first.
     * @return
     */
    const double * getXYValues() const {return xyValues; }

    /**
     * @brief xGridSize Size between probing points in the X axis.
     * @return
     */
    double xGridSize();

    /**
     * @brief yGridSize Size between probing points in the Y axis.
     * @return
     */
    double yGridSize();

    /**
     * @brief calculateOffset Calculate a new offset using the original Z data in the (0,0), the original offset and the new Z value in the (0,0).
     * @param newZValue New Z value in the (0,0) coordinate.
     * @return
     */
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
