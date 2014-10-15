#ifndef BASICGEOMETRY_H
#define BASICGEOMETRY_H

#include <math.h>

class Vector{
public:
    double x, y, z;
    Vector() {x = y = z = 0; }
    Vector(double xx, double yy, double zz) : x(xx), y(yy), z(zz) {}

    Vector(const Vector &v){x = v.x; y = v.y; z = v.z; }

    Vector operator+(const Vector &v) const {return Vector(x + v.x, y + v.y, z + v.z);}
    Vector& operator+=(const Vector &v) { x += v.x; y +=v.y; z += v.z; return *this; }

    Vector operator*(double f) const { return Vector(f*x, f*y, f*z); }

    Vector& operator*=(double f) { x *= f; y *= f; z *= f; return *this; }

    Vector operator/(double f) const { double inv = 1 / f; return Vector(x*inv, y*inv, z*inv); }
    Vector &operator/=(double f) { double inv = 1/f; x *= inv; y *= inv; z *= inv; return *this;}

    double lengthSquared() const { return x*x + y*y + z*z; }
    double length() const { return sqrt(lengthSquared()); }
};

inline Vector operator*(float f, const Vector &v) { return v*f; }
inline Vector normalize(const Vector &v) {return v / v.length();}

class Point{
public:
    double x, y, z;
    Point() { x = y = z = 0; }
    Point(double xx, double yy, double zz) : x(xx), y(yy), z(zz) {}
    Point(const Point &p) {x = p.x; y = p.y; z = p.z; }

    Point operator+(const Vector &v) const { return Point(x + v.x, y + v.y, z + v.z); }
    Point& operator+=(const Vector &v) { x += v.x; y += v.y; z += v.z; return *this; }

    Vector operator-(const Point &p) const { return Vector(x - p.x, y - p.y, z - p.z); }
    Point operator-(const Vector &v) const { return Point(x - v.x, y - v.y, z - v.z);}

    Point& operator-=(const Vector &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
};

inline double Distance(const Point &p1, const Point &p2) { return (p1 - p2).length();}
inline double DistanceSquared(const Point &p1, const Point &p2) { return (p1 - p2).lengthSquared();}


#endif // BASICGEOMETRY_H
