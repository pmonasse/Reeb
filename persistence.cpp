// SPDX-License-Identifier: MPL-2.0
/**
 * @file persistence.cpp
 * @brief Compute persistence map of bilinear image
 * @author Pascal Monasse <pascal.monasse@enpc.fr>
 * @date 2024
 */

#include "persistence.h"
#include <algorithm>
#include <vector>
#include <map>
#include <cassert>

struct Sample {
    short x,y;
    bool real;
    Sample(): x(-1), y(-1), real(true) {}
    Sample(short x0, short y0, bool r): x(x0), y(y0), real(r) {}

    bool operator==(const Sample& s) const {
        return real==s.real && x==s.x && y==s.y; }
    bool operator<(const Sample& s) const {
        if(real!=s.real)
            return real;
        return y<s.y || (y==s.y && x<s.x);
    }
};

inline size_t idx(const Sample& p, size_t w, size_t h) {
    assert(0<=p.x && (size_t)p.x<w && 0<=p.y && (size_t)p.y<h);
    short y = p.real? p.y: p.y+h;
    return p.x+y*w;
}

/// Functor to order samples by their value
struct CompareSamples {
    const float* im; size_t w, h;
    CompareSamples(const float* in, size_t w0, size_t h0)
    : im(in), w(w0), h(h0) {}
    bool operator()(const Sample& p, const Sample& q) const {
        float v1=im[idx(p,w,h)], v2=im[idx(q,w,h)];
        return  v1<v2 || (v1==v2 && p<q); 
    }
};

/// Image of virtual samples: they are the saddle points.
void fill_virtual_samples(const float* im, size_t w, size_t h, float* out) {
    for(size_t y=0; y<h; y++)
        for(size_t x=0; x<w; x++, im++) {
            float v = -1.0f;
            if(x+1<w && y+1<h) {
                float a=im[0], b=im[1], c=im[w], d=im[w+1];
                float min=a, max=d;
                if(min>max)
                    std::swap(min,max);
                int sb = b<min? -1: b>max? 1: 0;
                int sc = c<min? -1: c>max? 1: 0;
                if(sb*sc > 0)
                    v = (a*d-b*c)/(a+d-b-c);
            }
            *out++ = v;
        }
}

/// Find root of sample p and perform path compression.
/// s is the array of samples.
Sample* root(Sample* p, Sample* s, size_t w, size_t h) {
    Sample* q = s + idx(*p,w,h);
    if(q!=p) {
        q = root(q, s, w, h);
        *p = *q;
    }
    return q;
}

/// Make all pixels of the same component have the same canonical element.
void make_canonical(const float* im, Sample* par, size_t w, size_t h,
                    const Sample* orderedPos) {
    for(const Sample* pos = orderedPos+2*w*h; pos-- != orderedPos;) {
        Sample* p = par+idx(*pos,w,h);
        if(p->x<0) // end of normal samples
            break;
        const Sample* q = par+idx(*p,w,h); // q is the parent of p
        float v1=im[idx(*p,w,h)], v2=im[idx(*q,w,h)];
        if(v1 == v2)
            *p = *q;
    }
}

/// Indicate whether the sample at \a p is canonical. The parent relationship
/// \a par is already supposed to be made canonical with \c make_canonical.
bool is_canonical(const Sample& p, const float* im, const Sample* par,
                  size_t w, size_t h) {
    const Sample& parent = par[idx(p,w,h)];
    if(parent.x<0) // False sample
        return false;
    if(p==parent) // Root
        return true;
    float v = im[idx(p,w,h)];
    return (im[idx(parent,w,h)] != v);
}

struct Node {
    size_t parent;
    std::vector<size_t> children;
    float level, contrast;
    Node(float l): level(l), contrast(0) {}
};

/// Fill contrast information of node of index \a root in \a tree.
/// Propagate uptree, from leaves to parent.
float fill_contrast_uptree(std::vector<Node>& tree, size_t root) {
    Node& n = tree[root];
    // Recursive call on children
    float contrast=0;                // max
    for(std::vector<size_t>::const_iterator it=n.children.begin();
        it!=n.children.end(); ++it) {
        float c = fill_contrast_uptree(tree, *it);
        c += n.level - tree[*it].level;
        if(contrast<c)
            contrast = c;
    }
    n.contrast = contrast;
    return contrast;
}

/// Fill contrast information of node of index \a root in \a tree.
/// Propagate downtree, from parent to leaves.
void fill_contrast_downtree(std::vector<Node>& tree, size_t root) {
    Node& n = tree[root];
    // Recursive call on children
    float contrast=0;                // max
    std::vector<size_t> maxChildren; // argmax
    for(std::vector<size_t>::const_iterator it=n.children.begin();
        it!=n.children.end(); ++it) {
        float c = tree[*it].contrast;
        if(contrast<=c) {
            if(contrast!=c) {
                contrast = c;
                maxChildren.clear();
            }
            maxChildren.push_back(*it);
        }
    }
    // Propagate to dominant children
    for(std::vector<size_t>::const_iterator it=maxChildren.begin();
        it!=maxChildren.end(); ++it)
        tree[*it].contrast = n.contrast;
    // Recursive application to children
    for(std::vector<size_t>::const_iterator it=n.children.begin();
        it!=n.children.end(); ++it)
        fill_contrast_downtree(tree, *it);
}

/// Compute descending information and attributes.
std::vector<Node> compute_tree(const float* im, const Sample* par,
                               size_t w, size_t h, std::map<Sample,size_t>& m) {
    std::vector<Node> tree;
    // Enumerate nodes
    bool real=true;  
    for(size_t yy=0; yy<2*h; yy++) {
        size_t y = yy;
        if(h<=y) {
            y -= h;
            real = false;
        }
        for(size_t x=0; x<w; x++) {
            Sample p(x,y,real);
            if(is_canonical(p, im, par, w, h)) {
                m[p] = tree.size();
                tree.push_back( Node(im[idx(p,w,h)]) );
            }
        }
    }
    // Fill children
    size_t root=0;
    for(std::map<Sample,size_t>::iterator it=m.begin(); it!=m.end(); ++it) {
        const Sample& parent = par[idx(it->first,w,h)];
        if(parent == it->first)
            root = it->second;
        else {
            tree[m[parent]].children.push_back(it->second);
            tree[it->second].parent = m[parent];
        }
    }
    fill_contrast_uptree(tree, root);
    fill_contrast_downtree(tree, root);
    return tree;
}

void fill_persistence(const float* im, const Sample* par, size_t w, size_t h,
                      const std::vector<Node>& tree,
                      const std::map<Sample,size_t>& index,
                      float* out) {
    for(size_t y=0; y<h; y++)
        for(size_t x=0; x<w; x++) {
            Sample s(x,y,true);
            if(! is_canonical(s, im, par, w, h))
                s = par[idx(s,w,h)];
            *out++ = tree[index.at(s)].contrast;
        }
}

/// Neighborhood structure: real samples have 4 real neighbors and up to 4
/// virtual; virtual samples have 4 real samples.
const float dx[8+4] = {1, 0, -1,  0, 1, -1,  1, -1, /* virtual */ 0, 0, 1, 1};
const float dy[8+4] = {0, 1,  0, -1, 1,  1, -1, -1, /* virtual */ 0, 1, 0, 1};

void persistence(const float* in, size_t w, size_t h, float* out) {
    const size_t n=w*h;
    float* im = new float[2*n];
    std::copy(in, in+n, im);
    fill_virtual_samples(im, w, h, im+n);

    Sample* orderedPos = new Sample[2*n];
    Sample *q=orderedPos, *end=orderedPos+2*n;
    for(size_t y=0; y<h; y++) // Real samples
        for(size_t x=0; x<w; x++)
            *q++ = Sample(x,y,true);
    for(size_t y=0; y<h; y++) // Virtual samples
        for(size_t x=0; x<w; x++)
            *q++ = Sample(x,y,false);

    CompareSamples cmp(im,w,h);
    std::sort(orderedPos, end, cmp);
    Sample dumb((short)w-1,(short)h-1, false);
    assert(im[idx(dumb,w,h)] == -1.0f);
    Sample* p = std::upper_bound(orderedPos, end, dumb, cmp);

    Sample *par=new Sample[2*n], *zpar=new Sample[2*n];
    for(; p!=end; p++) {
        size_t k0 = idx(*p,w,h);
        par[k0] = zpar[k0] = *p;
        int nb=p->real?8:4, i0=p->real?0:8;
        for(int i=0; i<nb; i++) {
            int x=p->x+dx[i0+i], y=p->y+dy[i0+i];
            if(x<0 || w<=(size_t)x || y<0 || h<=(size_t)y) // outside image
                continue;
            size_t k = idx(Sample(x,y,i<4),w,h);
            if(zpar[k].x<0) // skip unprocessed neighbor
                continue;
            Sample* q = root(zpar+k, zpar, w, h);
            if(q != zpar+k0)
                *q = par[q-zpar] = *p;
        }
    }
    delete [] zpar;
    make_canonical(im, par, w, h, orderedPos);
    delete [] orderedPos;
    std::map<Sample,size_t> index;
    std::vector<Node> tree = compute_tree(im, par, w, h, index);
    fill_persistence(im, par, w, h, tree, index, out);
    delete [] im;
    delete [] par;
}
