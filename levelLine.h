// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file levelLine.h
 * @brief Extraction of level lines from an image
 * 
 * (C) 2011-2016, 2019, Pascal Monasse <pascal.monasse@enpc.fr>
 */

#ifndef LEVELLINE_H
#define LEVELLINE_H

#include <vector>
#include <iostream>

/// Type of point coordinates.
typedef float pt_t;

struct Point {
    pt_t x, y;
    Point() {}
    Point(pt_t x0, pt_t y0): x(x0), y(y0) {}
    bool operator==(const Point& p) const;
    bool operator!=(const Point& p) const;
};

inline bool Point::operator==(const Point& p) const {
    return (x==p.x && y==p.y); }
inline bool Point::operator!=(const Point& p) const {
    return !operator==(p); }

/// Vector addition
inline Point operator+(Point p1, Point p2) {
    return Point(p1.x+p2.x, p1.y+p2.y); }
inline Point& operator+=(Point& p1, Point p2) {
    p1.x += p2.x;
    p1.y += p2.y;
    return p1;
}

/// Inherit from this class to apply transform on the fly while drawing.
struct TransformPoint {
    virtual ~TransformPoint() {}
    virtual Point operator()(const Point& p) const { return p; }
};

/// Level line: a level and a polygonal line
struct LevelLine {
    pt_t level;
    std::vector<Point> line;
    enum Type { REGULAR=0, MIN, SADDLE, MAX };
    Type type;
    LevelLine(pt_t l, Type t=REGULAR): level(l), type(t) {}
    void fill(unsigned char* data, size_t w, size_t h,
              std::vector< std::vector<pt_t> >* inter=0) const;
};

std::ostream& operator<<(std::ostream& str, const LevelLine& line);

/// Abscissa (Inter.first) of intersection of level line of index (Inter.second)
typedef std::pair<pt_t,size_t> Inter;

void extract(const unsigned char* data, size_t w, size_t h,
             int ptsPixel,
             std::vector<LevelLine*>& ll,
             std::vector< std::vector<Inter> >* inter=0);

#endif
