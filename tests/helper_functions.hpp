
#ifndef wavplusplus_tests_helper_functions_hpp
#define wavplusplus_tests_helper_functions_hpp

#include <vector>

bool areEqual (double a, double b, double threshold);
bool areEqual (std::vector<double> const & a, std::vector<double> const & b, double threshold);
bool areEqual (std::vector<double> const & a, std::vector<double> const & b, size_t elementsToCompare, double threshold);
bool areEqual (std::vector<std::vector<double>> const & a, std::vector<std::vector<double>> const & b, double threshold);
void Print (std::vector<double> const & v);

bool areEqual (std::vector<uint8_t> const & a, std::vector<uint8_t> const & b);


#endif
