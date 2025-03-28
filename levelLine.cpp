// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file levelLine.cpp
 * @brief Extraction of level lines from an image
 * 
 * (C) 2011-2016, 2019, 2024-2025 Pascal Monasse <pascal.monasse@enpc.fr>
 */

#include "levelLine.h"
#include <stack>
#include <algorithm>
#include <cmath>
#include <cassert>

/// Quantification steps of singular levels. Safe up to width < 2^10 pixels.
/// 23 bits for epsilon machine: -8 bits for image depth, -6 bits for width.
const int QLEVEL = 1<<(23-8-6);
const pt_t DELTA_LEVEL = 1.0f/QLEVEL;
/// Quantized level of saddles.
pt_t qlevel(pt_t v) {
    pt_t intpart; // Integral part
    pt_t frac = std::modf(v, &intpart); // Fract part, quantified next line
    int s = (int)std::floor(frac*QLEVEL);
    s = std::max(2,std::min(QLEVEL-2,s));
    frac = s*DELTA_LEVEL;
    return intpart+frac;
}

/// South, East, North, West: directions of entry/exit in a dual pixel.
/// Left turn=+1. Right turn=-1. Opposite=+2.
typedef signed char Dir;
static const Dir S=0, /*E=1,*/ N=2 /*, W=3*/;

/// Vectors associated to the 4 directions
static const Point delta[] = {Point(0,+1), //S
                              Point(+1,0), //E
                              Point(0,-1), //N
                              Point(-1,0), //W
                              Point(0,+1)};//S again, to avoid modulo

/// Vector subtraction
inline Point operator-(Point p1, Point p2) {
    return Point(p1.x-p2.x, p1.y-p2.y);
}

/// Vector multiplication
inline Point operator*(pt_t f, Point p) {
    return Point(f*p.x, f*p.y);
}

/// Print list of points of the line (but not the level): x0 y0 x1 y1...
std::ostream& operator<<(std::ostream& str, const LevelLine& l) {
    std::vector<Point>::const_iterator it;
    for(it=l.line.begin(); it!=l.line.end(); ++it)
        str << it->x << " " << it->y << " ";
    return str;
}

/// Parameters of a hyperbola.
/// Inside the dual pixel, the level set has implicit equation
/// \f[ D*(x-xs)(y-ys)+N/D = l. \f]
/// This is true only if \f$D\neq0\f$ (otherwise we have just a line segment).
/// The center of the hyperbola (xs,ys) is a saddle point, its level is N/D.
/// Of interest, the vertex of the hyperbola is a point of maximal curvature.
/// It is located at
/// \f[(xs,ys)+(\pm\sqrt{|(Dl-N)/D^2|},\pm\sqrt{|(Dl-N)/D^2|}).\f]
/// The signs are determined by the fact that the vertex is in the same quadrant
/// as the input point p with respect to (xs,ys).
/// The equation of hyperbola is written
/// \f[ (x-xs)(y-ys) = \delta. \f]
class Hyperbola {
public:
    int num, denom; /// The saddle value is num/denom
    Point s; ///< Saddle point=center of hyperbola
    Point v; ///< Vertex of hyperbola=point of maximal curvature
    pt_t delta; ///< Parameter of hyperbola (sqrt(2*delta) = semi major axis)

    Hyperbola(const Point& pos, const Point& p, unsigned char lev[4], pt_t l);
    bool valid() const { return (denom!=0); }
    bool vertex_in_dual_pixel(const Point& p) const;
    void sample(const Point& p1, const Point& p2, int ptsPixel,
                std::vector<Point>& line) const;
private:
    static int sign(pt_t f) { return (f>0)? +1: -1; }
};

/// Decompose hyperbola branch.
/// \param pos the top-left vertex of the dual pixel.
/// \param p a point on an edgel of the dual pixel and on the hyperbola.
/// \param level the levels at the four vertices of the dual pixel.
/// \param l level.
/// The hyperbola can be degenerate (a segment), in which case \c s, \c v and
/// \c delta make no sense. The method \c valid() must be used to check.
Hyperbola::Hyperbola(const Point& pos, const Point& p,
                     unsigned char level[4], pt_t l) {
    num   =  level[0]*level[2] - level[1]*level[3];
    denom = (level[0]+level[2])-(level[1]+level[3]);
    delta = 0;
    if(denom == 0)
        return; // Degenerate hyperbola
    pt_t d = 1.0/denom;
    s.x = pos.x + (level[0]-level[1])*d;
    s.y = pos.y + (level[0]-level[3])*d;
    delta = (denom*l-num)*(d*d);
    d = sqrt(std::abs(delta));
    v.x = s.x + sign(p.x-s.x)*d;
    v.y = s.y + sign(p.y-s.y)*d;
    if(denom<0) { // Optim for later: transform a<num/denom into a*denom<num
        num = -num;
        denom=-denom;
    }
}

/// Tell if the vertex of the hyperbola branch is inside the dual pixel of
/// top-left corner \a p.
bool Hyperbola::vertex_in_dual_pixel(const Point& p) const {
    return valid() && (p.x<v.x && v.x<p.x+1 && p.y<v.y && v.y<p.y+1);
}

/// Sample branch of hyperbola from p1 to p2 of equation (x-xs)(y-ys)=delta:
/// [2]Algorithm 3.
/// \param p1 start point.
/// \param p2 end point.
/// \param ptsPixel number of points of discretization per pixel.
/// \param[out] line where the sampled points are stored.
void Hyperbola::sample(const Point& p1, const Point& p2, int ptsPixel,
                       std::vector<Point>& line) const {
    if(ptsPixel<2) return;
    Point p = p2-p1;
    if(p.x<0) p.x=-p.x;
    if(p.y<0) p.y=-p.y;
    if(p.x>p.y) { // Uniform sample along x
        int n = ceil(p.x*ptsPixel);
        pt_t dx = (p2.x-p1.x)/n;
        p = p1;
        for(int i=1; i<n; i++) {
            p.x += dx;
            p.y = s.y + delta/(p.x-s.x);
            line.push_back(p);
        }
    } else { // Uniform sample along y
        int n = ceil(p.y*ptsPixel);
        pt_t dy = (p2.y-p1.y)/n;
        p = p1;
        for(int i=1; i<n; i++) {
            p.y += dy;
            p.x = s.x + delta/(p.y-s.y);
            line.push_back(p);
        }
    }
}

/// A mobile dual pixel, square whose vertices are 4 data points.
/// This is the main structure to extract a level line, moving from dual pixel
/// to an adjacent one until coming back at starting point. The entry direction
/// of the level line is recorded: south means it enters from the top horizontal
/// edgel, east from the right vertical, north from the bottom horizontal and
/// west from the right.
/// The object stores the levels at its 4 vertices (data points of the image),
/// in clockwise order starting from the top left vertex.
class DualPixel {
public:
    DualPixel(Point& p, pt_t l, const unsigned char* im, size_t w);
    void follow(Point& p, pt_t l, int ptsPixel, std::vector<Point>& line);
    bool mark_visit(std::vector<bool>& visit,
                    std::vector< std::vector<Inter> >* inter, size_t idx,
                    const Point& p) const;
private:
    const unsigned char* _im; ///< The image stored as 1D array.
    const size_t _w; ///< Number of columns of image.
    unsigned char _level[4]; /// The levels at the 4 data points.
    Point _pos; /// The position of the top-left vertex of the dual pixel.
    Dir _d; /// Direction of entry into dual pixel.

    void update_levels();
    Point move(pt_t l, int snum, int sdenom);
};

/// Return x for y=v on line joining (0,v0) and (1,v1).
inline pt_t linear(pt_t v0, pt_t v, pt_t v1) {
    return (v-v0)/(v1-v0);
}

/// Constructor.
/// \param[in,out] p the edgel endpoint at the right of incoming direction.
/// \param l the level of the level line.
/// \param im the values of pixels in a 1D array.
/// \param w the number of pixel columns in \a im.
/// The incoming direction is always supposed to be south, so the level line is
/// crossing the edgel from \a p to \a p+(1,0). It means the starting point of
/// the level line is at \a p+(x,0), with 0<x<1. As output, \a p is moved to
/// this position.
DualPixel::DualPixel(Point& p, pt_t l, const unsigned char* im, size_t w)
: _im(im), _w(w), _pos(p), _d(S) {
    update_levels();
    if(_level[_d]>l && l>_level[(_d+3)%4]) {
        _d = N;
        --_pos.y; ++p.x;
        update_levels();
    }
    p += linear(_level[_d],l,_level[(_d+3)%4])*delta[_d+1];
}

/// Update levels at vertices.
void DualPixel::update_levels() {
    size_t ind = (size_t)_pos.y*_w+(size_t)_pos.x;
    _level[0] = _im[ind];    _level[3] = _im[ind+1];
    _level[1] = _im[ind+_w]; _level[2] = _im[ind+_w+1];
}

/// Move to next adjacent dual pixel: [2]Algorithm 2.
/// \param l the level of the level line
/// \param snum numerator of saddle level
/// \param sdenom denominator of saddle level
/// \return subpixel entry point in new dual pixel (=exit point of old one)
/// Only the saddle level (snum/sdenom) may be used, but most of the time it is
/// not. Pass two parameters in order not to pay an unnecessary division.
Point DualPixel::move(pt_t l, int snum, int sdenom) {
    bool left  = (l>_level[(_d+2)%4]); // Is there an exit at the left?
    bool right = (l<_level[(_d+1)%4]); // Is there an exit at the right?
    if(left && right) { // Disambiguate saddle point
        right = (l*sdenom<snum); // sdenom>0, so equivalent to l<num/denom
        left = !right;
    }
    // update direction
    if(left  && ++_d>3) _d=0;
    if(right && --_d<0) _d=3;
    // update top-left vertex
    _pos += delta[_d];
    update_levels();

    pt_t coord = linear(_level[_d], l, _level[(_d+3)%4]);
    Point p = _pos;
    for(Dir d=0; d<_d; d++) p += delta[d];
    p += coord*delta[_d+1]; // Safe: delta[4]==delta[0]
    return p;
}

/// The dual pixel is moved to the adjacent one. Find exit point of level line
/// entering at \a p in the dual pixel. The level line is sampled up to there. 
/// \param[in,out] p entry point into the dual pixel
/// \param l level of the level line
/// \param ptsPixel number of points of discretization per pixel.
/// \param[out] line intermediate samples stored here.
void DualPixel::follow(Point& p, pt_t l, int ptsPixel,
                       std::vector<Point>& line) {
    assert(_level[_d]<l && l<_level[(_d+3)%4]);
    // 1. Compute hyperbola equation
    Hyperbola h(_pos, p, _level, l);
    bool vInside = h.vertex_in_dual_pixel(_pos);
    // 2. Move dual pixel to new position
    Point pIni = p; // Keep track of entry point before moving to exit
    p = move(l, h.num, h.denom);
    // 3. Sample hyperbola in previous dual pixel position
    if(h.valid() && ptsPixel>0) { // Do not sample if not hyperbola (straight)
        if(std::abs(h.delta) < 1.0e-2) { // Saddle level: one or two segments
            if(vInside)
                line.push_back(h.v); // Put vertex only (almost saddle point)
            return;
        }
        if(vInside) { // Sample from entry point to vertex of hyperbola
            h.sample(pIni, h.v, ptsPixel, line);
            line.push_back(pIni=h.v);
        }
        h.sample(pIni, p, ptsPixel, line); // Sample until end point
    }
}

/// Mark the edge as "visited", return \c false if already visited.
/// \param visit stores the edgels traversed from the south at current level.
/// \param inter (optional) rows of image traversed are marked with \a idx.
/// \param idx a unique identifier for the level line.
/// \param p the current position in the tracking of the level line.
/// \return whether the tracking must continue (loop not closed yet).
/// When we go through a horizontal data row and going south, we store the
/// visit. If the edgel was already visited at current level, we came back
/// at starting point and must stop.
bool DualPixel::mark_visit(std::vector<bool>& visit,
                           std::vector< std::vector<Inter> >* inter,
                           size_t idx, const Point& p) const {
    bool cont=true;
    if(_d==S || _d==N) {
        size_t i = (size_t)_pos.y*_w+(size_t)_pos.x;
        if(_d==N)
            i += _w;
        cont = !visit[i];
        visit[i] = true;
    }
    if(inter && cont && (_d==S||_d==N))
        (*inter)[(size_t)p.y].push_back( Inter(p.x,idx) );
    return cont;
}

/// Extract level line passing through a given starting point. 
/// \param data the values of pixels in a 1D array.
/// \param w the number of pixel columns in \a data.
/// \param visit array to store the visited explored horizontal edgels.
/// \param ptsPixel number of points of discretization per pixel.
/// \param p the starting point.
/// \param[in,out] ll the level line: level already stored, find its line.
/// \param idx a unique identifier for the level line.
/// \param inter[out] (optional) rows of image traversed are marked with \a idx.
/// \a inter is used to recover the tree hierarchy at the end, could be
/// omitted if the tree is not required, in which case \a idx is unused.
static void extract(const unsigned char* data, size_t w,
                    std::vector<bool>& visit, int ptsPixel,
                    Point p, LevelLine& ll, size_t idx,
                    std::vector< std::vector<Inter> >* inter) {
    DualPixel dual(p, ll.level, data, w);
    while(true) {
        ll.line.push_back(p);
        if(! dual.mark_visit(visit,inter,idx,p))
            break;
        dual.follow(p,ll.level,ptsPixel,ll.line);
    }
}

/// Find regional maximum (or minimum if max=false) containing (x,y) in \a im.
/// \a vu initially tags pixels that cannot take part, augmented then with
/// pixels explored during the process.
static bool find_extremum(const unsigned char* im, size_t w, size_t h,
                          size_t x, size_t y, bool max,
                          bool* vu, std::vector<Point>& V) {
    unsigned char level=im[x+y*w];
    vu[x+y*w] = true;
    std::stack<Point> S;
    S.push( Point((pt_t)x,(pt_t)y) );
    bool success = true;
    while(! S.empty()) {
        Point p = S.top(); S.pop();
        V.push_back(p);
        for(int i=0; i<4; i++) {
            Point q = p+delta[i];
            size_t idx = (size_t)q.x+(size_t)q.y*w;
            if(im[idx]==level) {
                if(q.x==0 || (size_t)q.x+1==w || q.y==0 || (size_t)q.y+1==h)
                    success = false;
                else if(! vu[idx]) {
                    vu[idx] = true;
                    S.push(q);
                }
            } else if(max != (im[idx]<level))
                success = false;
        }
    }
    return success;
}

/// Handle extrema of the bilinear image.
void handle_extrema(const unsigned char* im, size_t w, size_t h,
                    int ptsPixel,
                    std::vector<LevelLine*>& ll,
                    std::vector<bool>& visit,
                    std::vector< std::vector<Inter> >* inter) {
    bool* vu = new bool[w*h];
    std::fill(vu, vu+w*h, false);
    for(size_t y=1; y+1<h; y++) {
        size_t idx = y*w+1;
        for(size_t x=1; x+1<w; x++, idx++) {
            if(vu[idx] || im[idx] == im[idx+1])
                continue;
            unsigned char level=im[idx];
            bool max = (im[idx+1]<level);
            std::vector<Point> V;
            if(! find_extremum(im,w,h, x,y,max, vu, V))
                continue;
            pt_t v = (max? level-DELTA_LEVEL: level+DELTA_LEVEL);
            for(std::vector<Point>::iterator it=V.begin();
                it!=V.end(); ++it) {
                size_t idx2 = (size_t)it->x+(size_t)it->y*w;
                if(im[idx2+1] != level && !visit[idx2]) {
                    LevelLine::Type t = max? LevelLine::MAX: LevelLine::MIN;
                    LevelLine* line = new LevelLine(v,t);
                    extract(im,w, visit, ptsPixel, *it, *line,ll.size(), inter);
                    ll.push_back(line);
                }
            }
            std::fill(visit.begin(), visit.end(), false);
        }
    }
    delete [] vu;
}

/// Structure to record all saddle points inside the image.
struct Saddle {
    size_t x, y; ///< Top-left corner of sample square
    pt_t value; ///< Level of saddle
    Saddle(size_t x0, size_t y0, pt_t v): x(x0), y(y0), value(v) {}
};
bool operator<(const Saddle& s1, const Saddle& s2) {
    return s1.value < s2.value;
}

/// If saddle in unit square of top-left corner (x,y), return its level.
static bool level_saddle(const unsigned char* im, size_t w, size_t h,
                         size_t x, size_t y, pt_t& v) {
    if(x+1>=w || y+1>=h)
        return false;
    size_t idx0=x+w*y;
    unsigned char a=im[idx0], b=im[idx0+1], c=im[idx0+w], d=im[idx0+w+1];
    unsigned char min=a, max=d;
    if(min>max)
        std::swap(min,max);
    int sb = b<min? -1: b>max? 1: 0;
    int sc = c<min? -1: c>max? 1: 0;
    if(sb*sc <= 0)
        return false;
    v = (a*d-b*c)/pt_t(a+d-b-c);
    return true;
}

/// Find all saddle points of the bilinear image.
std::vector<Saddle> find_saddles(const unsigned char* im, size_t w, size_t h) {
    std::vector<Saddle> S;
    for(size_t y=0; y<h; y++)
        for(size_t x=0; x<w; x++) {
            pt_t v;
            if(level_saddle(im,w,h, x,y, v))
                S.push_back( Saddle(x,y,v) );
        }
    return S;
}

/// Handle saddle points.
void handle_saddles(const unsigned char* im, size_t w, size_t h,
                    int ptsPixel,
                    std::vector<LevelLine*>& ll,
                    std::vector<bool>& visit,
                    std::vector< std::vector<Inter> >* inter) {
    std::vector<Saddle> S = find_saddles(im,w,h);
    std::sort(S.begin(), S.end());
    for(std::vector<Saddle>::const_iterator it=S.begin(); it!=S.end();) {
        pt_t v = qlevel(it->value); // Handle together all at same quant. level
        for(; it!=S.end() && qlevel(it->value)==v; ++it) {
            for(size_t i=0; i<=1; i++)
                if(! visit[it->x+(it->y+i)*w]) {
                    LevelLine* line = new LevelLine(v, LevelLine::SADDLE);
                    Point p((pt_t)it->x,(pt_t)it->y+i);
                    extract(im,w, visit, ptsPixel, p, *line, ll.size(), inter);
                    ll.push_back(line);
                }
        }
        std::fill(visit.begin(), visit.end(), false);
    }
}

/// Level lines extraction algorithm.
/// \param im the values of pixels in a 1D array.
/// \param w the number of pixel columns in \a data.
/// \param h the number of pixel lines in \a data.
/// \param ptsPixel number of points of discretization per pixel.
/// \param[out] ll storage for the extracted level lines.
/// \param inter[out] (optional) rows of image traversed by ll are marked.
void extract(const unsigned char* im, size_t w, size_t h,
             int ptsPixel,
             std::vector<LevelLine*>& ll,
             std::vector< std::vector<Inter> >* inter) {
    std::vector<bool> visit(w*h, false);
    if(inter) {
        assert(inter->empty());
        inter->resize(h);
    }
    handle_extrema(im,w,h, ptsPixel, ll, visit, inter);
    handle_saddles(im,w,h, ptsPixel, ll, visit, inter);
}
