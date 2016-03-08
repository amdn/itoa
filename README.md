# itoa
Very fast integer to ASCII conversion, C++14 template meta-programming implementation, any size integer 8,16,32,64-bit, signed and unsigned, forward (left-justified) or reverse (right justified)

Compile and link itoa.cpp with your application.

itoa.h      - programming interface
itoa_impl.h - implementation

Two function templates are provided to print the integer going forward at the given buffer (returns pointer to the past-the-end character of the string) or reverse given a pointer to the end (characters stored to the left, lower addresses from given pointer, returns pointer to beginning of string).

```c++
// print forward, return pointer to past-the-end
template<typename I> char* itoa_fwd (I i,char *buffer);

// print reverse, return pointer to first character
template<typename I> char* itoa_rev (I i,char *buffer);
```
