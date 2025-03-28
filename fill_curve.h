// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file fill_curve.h
 * @brief Fill the interior of a closed curve in an image
 * 
 * (C) 2011-2014, 2019, Pascal Monasse <pascal.monasse@enpc.fr>
 */

#ifndef FILL_CURVE_H
#define FILL_CURVE_H

#include "levelLine.h"

template <typename T>
void fill_curve(const std::vector<Point>& line, T v, T* im, int w, int h,
                const TransformPoint& t=TransformPoint());

// Templates must have their implementation nearby
#include "fill_curve.cpp"

#endif
