
// Works with std::vector, C-style arrays, and std::array.
// If the number of samples desired is given, an attempt will be made to read or write
// that number of samples, else the size of the container will be used as the number
// of samples to read or write.
// For C-style arrays whose size is known at compile time, (even arrays created with new?),
// both options exist. For dynamic (?) C-style arrays, the number of sample to read/write must be supplied.
//
// writer.write (vec);
// writer.write (vec, samplesRequested);
//
// writer.write (stdArray);
// writer.write (stdArray, samplesResquested);
//
// writer.write (C_array_fixed);
// writer.write (C_array_fixed, samplesRequested);
//
// writer.write (C_array_dynamic, samplesRequested);
//
// The above options also apply to reading with Reader::read().
