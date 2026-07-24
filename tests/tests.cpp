// WAV++ (c) 2025-2026 David Adams
// SPDX-License-Identifier: MIT

#include "TestLog.hpp"
#include "helper_functions.hpp"

#include "writing/WritingTests.hpp"
#include "reading/ReadingTests.hpp"
#include "round_trip/RoundTripTests.hpp"
#include "utilities/UtilitiesTests.hpp"

#include <iostream>

int main ()
{
    TestLog log;

    TestWriting   (log);
    TestReading   (log);
    TestRoundTrip (log);
    TestUtilities (log);

    std::cout << log << std::endl;

    return (log.failures() == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
