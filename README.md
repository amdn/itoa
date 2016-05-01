# itoa
World's fastest integer to ASCII conversion, C++14 template meta-programming implementation, any size integer 8,16,32,64-bit, signed or unsigned, forward (left-justified) or reverse (right justified).  If you know of a faster implementation, let me know.

Compile and link itoa.cpp with your application.

    itoa.h      - programming interface
    itoa_impl.h - implementation
    itoa.cpp    - definition of digits lookup table

Two function templates are provided to print the integer (without a NUL terminator)
  * left-justified, going forward at the given buffer - returns pointer to the past-the-end character of the string
  * right-justified, given a pointer to the end store in reverse - returns pointer to beginning of string

```c++
// print forward, return pointer to past-the-end
template<typename I> char* itoa_fwd (I i,char *buffer);

// print reverse, return pointer to first character
template<typename I> char* itoa_rev (I i,char *buffer);
```
