
#include "TestLog.hpp"
#include <cmath>
#include <ios>
#include <iomanip>
#include <sstream>
#include <cassert>

void TestLog::Record(bool result, std::string const & description)
{
    descriptions.emplace_back(description);
    results.emplace_back(result);
    if(result==true) successes_++;
    else failures_++;
}
int TestLog::successes() const { return successes_; }
int TestLog::failures() const { return failures_; }
int TestLog::testCount() const { return successes_+failures_; }
double TestLog::successRate() const
{
    return successes() / static_cast<double>(testCount());
}
double TestLog::failureRate() const
{
    return failures() / static_cast<double>(testCount());
}
std::string TestLog::formattedResults() const
{
    std::string output;
    assert (descriptions.size()==results.size());
    output += "\nTest results\n";
    output += "----------------\n";
    for(size_t i=0; i<descriptions.size(); ++i)
    {
        output += (results[i]==true ? "pass - " : "fail - ") + descriptions[i] + "\n";
    }

    if (failures() == 0)
    {
        output += "\n" + std::to_string(testCount()) + " tests in total.\n";
        output += "\n\n*** All tests pass. ***";
    }
    else
    {
        output += "\nFailed Tests\n";
        output += "------------\n";
        for (size_t i=0; i<descriptions.size(); ++i)
        {
            if (results[i]==false) output +=  descriptions[i] + "\n";
        }

        //output += "\n\n!!!!!!!!!!!!!!!!!!!!!!!!!!";
        output += "\n\n" + std::to_string(testCount()) + " tests in total.\n";
        output += "\n\n*** " + std::to_string(failures()) + " *** " + (failures()==1 ? " test" : " tests") + " failed.\n";
        output += "failure rate: " + std::to_string(100. * failureRate()) + "%";
        //output += "\n--------------------------";
    }
    output += "\n\n";
    return output;
}
