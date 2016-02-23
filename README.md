# itoa
Very fast integer to ASCII conversion, C++14 template meta-programming implementation, any size integer 8,16,32,64-bit, signed and unsigned, forward (left-justified) or reverse (right justified)

Compile and link itoa.cpp with your application.

Implementation and API is in itoa.h header.

Two function templates are provided to print the integer going forward at the given buffer (returns pointer to the past-the-end character of the string) or reverse given a pointer to the end (characters stored to the left, lower addresses from given pointer, returns pointer to beginning of string).

template<typename I, size_t N = sizeof(I)> char* itoa_fwd (I i,char *p);

template<typename I, size_t N = sizeof(I)> char* itoa_rev (I i,char *p);

