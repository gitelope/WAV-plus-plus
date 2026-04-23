
#include "TestLog.hpp"
#include "helper_functions.hpp"

#include "writing/WritingTests.hpp"
#include "reading/ReadingTests.hpp"
#include "utilities/UtilitiesTests.hpp"

#include <iostream>

int main ()
{
    TestLog log;

    TestWriting   (log);
    TestReading   (log);
    TestUtilities (log);

    std::cout << log << std::endl;

    return EXIT_SUCCESS;
}
