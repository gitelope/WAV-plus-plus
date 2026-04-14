
#ifndef wavplusplus_tests_helper_functions_hpp
#define wavplusplus_tests_helper_functions_hpp

#include <cassert>
#include <vector>

/*
bool areEqual (double a, double b, double threshold);
bool areEqual (std::vector<double> const & a, std::vector<double> const & b, double threshold);
bool areEqual (std::vector<double> const & a, std::vector<double> const & b, size_t elementsToCompare, double threshold);
bool areEqual (std::vector<std::vector<double>> const & a, std::vector<std::vector<double>> const & b, double threshold);
void Print (std::vector<double> const & v);
*/

template <typename T>
bool areEqual (std::vector<T> const & a, std::vector<T> const & b)
{
    assert (a.size() == b.size());
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (a[i] != b[i]) return false;
    }
    return true;
}

void Compare (bool passes, std::string const & failure, std::string & failures);

#endif
