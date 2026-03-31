
#include "helper_functions.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

/*
// test Buffers for equality by comparing difference
// to a threshold.
bool areEqual (std::vector<double> const & a,
               std::vector<double> const & b,
               const double threshold)
{
    assert (a.size() == b.size());

    for (size_t i = 0; i < a.size(); ++i)
        if (std::abs (a[i] - b[i]) > threshold)
            return false;
    return true;
}

// If elements to compare is greater than the size of one of the vectors,
// function will compare as many elements as possible.
bool areEqual (std::vector<double> const & a,
               std::vector<double> const & b,
               size_t elementsToCompare,
               const double threshold)
{
    const auto limit = std::min (elementsToCompare, std::min (a.size(), b.size()));
    for (size_t i = 0; i < limit; ++i)
        if (std::abs (a[i] - b[i]) > threshold)
            return false;

    return true;
}

bool areEqual (const double a,
               const double b,
               const double threshold)
{
    return std::abs (a - b) <= threshold;
}

bool areEqual (std::vector<std::vector<double>> const & a, std::vector<std::vector<double>> const & b, double threshold)
{
    assert (a.size() == b.size());
    for (size_t i = 0; i < a.size(); ++i)
        if (! areEqual (a[i], b[i], threshold))
            return false;

    return true;
}

void Print (std::vector<double> const & v)
{
    for (auto const & element : v) std::cout << element << " ";
    std::cout << std::endl;
}
*/

