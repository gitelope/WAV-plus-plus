
#ifndef wavplusplus_tests_TestLog_hpp
#define wavplusplus_tests_TestLog_hpp

#include <string>
#include <vector>

class TestLog
{
public:
    void Record(bool result, std::string const & testDescription);
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

#endif
