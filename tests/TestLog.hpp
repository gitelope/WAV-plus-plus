// WAV++ (c) 2025 David Adams
// SPDX-License-Identifier: MIT

#ifndef wavplusplus_tests_TestLog_hpp
#define wavplusplus_tests_TestLog_hpp

#include <cassert>
#include <cmath>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <vector>

class TestLog
{
public:
    void Record (bool result, std::string const & testDescription);
    int successes() const;
    int failures() const;
    int testCount() const;
    double successRate() const;
    double failureRate() const;
    std::string formattedResults() const;
private:
    std::vector<std::string> descriptions;
    std::vector<char> results; // b/c vector<bool> is no good.
    int successes_ {0};
    int failures_ {0};
};

inline void TestLog::Record (bool result, std::string const & description)
{
    descriptions.emplace_back (description);
    results.emplace_back (result);
    if (result == true) successes_++;
    else failures_++;
}

inline int TestLog::successes() const { return successes_; }
inline int TestLog::failures() const { return failures_; }
inline int TestLog::testCount() const { return successes_+failures_; }

inline double TestLog::successRate() const
{
    return successes() / static_cast<double> (testCount());
}

inline double TestLog::failureRate() const
{
    return failures() / static_cast<double> (testCount());
}

inline std::string TestLog::formattedResults() const
{
    std::string output;
    assert (descriptions.size() == results.size());
    output += "\ntest results\n";
    output += "------------\n";
    for(size_t i=0; i<descriptions.size(); ++i)
    {
        output += (results[i] == 1 ? "pass - " : "fail - ") + descriptions[i] + "\n";
    }

    if (failures() == 0)
    {
        output += "\n" + std::to_string (testCount()) + " tests in total.\n";
        output += "\n\n*** All tests pass. ***";
    }
    else
    {
        output += "\nFailed Tests\n";
        output += "------------\n";
        for (size_t i=0; i<descriptions.size(); ++i)
        {
            if (results[i] == 0) output += descriptions[i] + "\n";
        }
        output += "\n\n" + std::to_string (testCount()) + " tests in total.\n";
        output += "\n\n*** " + std::to_string (failures()) + " *** " + (failures() == 1 ? " test" : " tests") + " failed.\n";
        output += "failure rate: " + std::to_string (100. * failureRate()) + "%";
    }
    output += "\n\n";
    return output;
}

inline std::ostream & operator<< (std::ostream & os, TestLog const & log)
{
    return os << log.formattedResults();
}

#endif
