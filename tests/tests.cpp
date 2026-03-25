
#include "TestLog.hpp"
#include "writing/WritingTests.hpp"
#include <iostream>

int main ()
{
    TestLog log;

    TestWriting (log);

    std::cout << log.formattedResults() << std::endl;

    return EXIT_SUCCESS;
}
