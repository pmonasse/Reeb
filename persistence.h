// SPDX-License-Identifier: MPL-2.0
/**
 * @file persistence.h
 * @brief Compute persistence map of bilinear image
 * @author Pascal Monasse <pascal.monasse@enpc.fr>
 * @date 2024
 */

#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <cstddef>

void persistence(const float* im, size_t w, size_t h, float* out);

#endif
