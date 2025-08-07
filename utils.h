/**
 * @file utils.h
 * @brief Contains utility functions and constants used throughout the application.
 */

#pragma once

#include <unordered_map>

/**
 * @brief A map for converting score differences from par to Stableford points.
 *
 * The key is the difference from par (e.g., -1 for a birdie), and the value is
 * the corresponding number of Stableford points.
 */
const std::unordered_map<int, int> stableford_conversion = {
    {-5, 12},
    {-4, 10},
    {-3, 8},
    {-2, 6},
    {-1, 4},
    {0, 2},
    {1, 1},
    {2, 0},
    {3, -1}
};