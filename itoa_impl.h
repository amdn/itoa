//=== itoa.h - Fast integer to ascii conversion                   --*- C++ -*-//
//
// The MIT License (MIT)
// Copyright (c) 2016 Arturo Martin-de-Nicolas
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
//     The above copyright notice and this permission notice shall be included
//     in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//===----------------------------------------------------------------------===//

#ifndef DEC_ITOA_IMPL_H
#define DEC_ITOA_IMPL_H

#include "itoa.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

namespace dec_ {

    using uint128_t = unsigned __int128;

    // Using a lookup table to convert binary numbers from 0 to 99
    // into ascii characters as described by Andrei Alexandrescu in
    // https://www.facebook.com/notes/facebook-engineering/
    //         three-optimization-tips-for-c/10151361643253920/

    extern const std::array<char,200> digits;
    static inline uint16_t const& dd(uint8_t u) {
        return reinterpret_cast<uint16_t const*>(digits.data())[u];
    }

    enum Direction {Fwd,Rev};

    template<typename T> static constexpr T pow10(size_t x) {
        return x ? 10*pow10<T>(x-1) : 1;
    }

    // Division by a power of 10 is implemented using a multiplicative inverse.
    // This strength reduction is also done by optimizing compilers, but
    // presently the fastest results are produced by using the values
    // for the muliplication and the shift as given by the algorithm
    // described by Agner Fog in "Optimizing Subroutines in Assembly Language"
    //
    // http://www.agner.org/optimize/optimizing_assembly.pdf
    //
    // "Integer division by a constant (all processors)
    // A floating point number can be divided by a constant by multiplying
    // with the reciprocal. If we want to do the same with integers, we have
    // to scale the reciprocal by 2n and then shift the product to the right
    // by n. There are various algorithms for finding a suitable value of n
    // and compensating for rounding errors. The algorithm described below
    // was invented by Terje Mathisen, Norway, and not published elsewhere."

    template<typename UInt, bool A, UInt M, unsigned S> struct MulInv {
        using type = UInt;
        static constexpr bool     a{ A };
        static constexpr UInt     m{ M };
        static constexpr unsigned s{ S };
    };
    template<int, int, class...> struct UT;
    template<int N, class T, class...Ts> struct UT<N,N,T,Ts...> { using U = T; };
    template<int N, int M, class T, class...Ts> struct UT<N,M,T,Ts...> {
        using U = typename UT<N,2*M,Ts...>::U;
    };
    template<int N> using MI = typename UT<N,1,
        MulInv< uint8_t,   0,                  205U, 11 >,
        MulInv< uint16_t,  1,                41943U, 22 >,
        MulInv< uint32_t,  0,           3518437209U, 45 >,
        MulInv< uint64_t,  0, 12379400392853802749U, 90 >,
        MulInv< uint128_t, 0, 0, 0 >
        >::U;
    template<int N> using U = typename MI<N>::type;

    // struct QR holds the result of dividing an unsigned N-byte variable
    // by 10^N resulting in
    template<size_t N> struct QR {
        U<N> q;   // quotient with fewer than 2*N decimal digits
        U<N/2> r; // remainder with at most N decimal digits
    };
    template<size_t N> QR<N> static inline split( U<N> u ) {
        constexpr MI<N> mi{};
        U<N> q = (mi.m * (U<2*N>(u)+mi.a)) >> mi.s;
        return { q, U<N/2>( u - q * pow10<U<N/2>>(N) ) };
    }

    template < Direction D >
        struct convert
        {
            enum Magnitude { NotLarge, M=NotLarge, MaybeLarge };
            enum ZFill { NoFill, Z };

            // output the digits in either a forward or reverse direction
            // using memcpy so compiler handles alignment on target architecture.
            // This will typically generate one store to memory with an optimizing
            // compiler and a target architecture that supports unaligned access.
            template<typename T>
            static inline char* out(char* p, T&& obj) {
                if (D==Rev) p -= sizeof(T);
                memcpy(p,reinterpret_cast<const void*>(&obj),sizeof(T));
                if (D==Fwd) p += sizeof(T);
                return p;
            }

            // i2a_small
            // recursively handle values of "u" less than 10^N,
            // where N is the # bytes in "u"
            template<size_t N>
            static inline char* i2a_small(char* p, U<N> u, ZFill z) {
                return ( z == NoFill
                         ?( i2a(p,U<N/2>(u),M,z))
                         :( D==Fwd
                            ?( i2a( i2a(p,U<N/2>(0),M,z),U<N/2>(u),M,z))
                            :( i2a( i2a(p,U<N/2>(u),M,z),U<N/2>(0),M,z))));
            }

            // i2a_medium
            // recursively handle values less than 10^2*N, where x contains
            // quotient and remainder after division by 10^N
            template<size_t N>
            static inline char* i2a_medium(char* p, ZFill z, QR<N> x) {
                return ( D==Fwd
                         ?( i2a( i2a(p,U<N/2>(x.q),M,z),x.r,M,Z))
                         :( i2a( i2a(p,x.r,M,Z),U<N/2>(x.q),M,z)));
            }

            // i2a_large
            // recursively handle values greater than or equal to 10^2*N
            // where x contains quotient and remainder after division by 10^N
            template<size_t N>
            static inline char* i2a_large(char* p, ZFill z, QR<N> x) {
                auto y = split<N>(x.q);
                return ( D==Fwd
                         ?( i2a( i2a( i2a(p,U<N/2>(y.q),M,z),y.r,M,Z),x.r,M,Z))
                         :( i2a( i2a( i2a(p,x.r,M,Z),y.r,M,Z),U<N/2>(y.q),M,z)));
            }

            // i2a: split the value along a decimal digit boundary based on size
            // of operand in bytes (N)
            //     - small if value is less than N decimal digits, else
            //     - medium if value is less than 2*N decimal digits, else
            //     - large
            template<typename UInt, size_t N=sizeof(UInt)>
            static inline char* i2a(char* p, UInt u, Magnitude mag, ZFill z) {
                if (u < pow10<U<N>>(N)) return i2a_small<N>(p,u,z);
                auto x = split<N>(u);
                if ( mag == NotLarge || u < pow10<U<N>>(2*N)) return i2a_medium<N>(p,z,x);
                return i2a_large<N>(p,z,x);
            }

            // i2a recursion base case,
            // selected by overload resolution when "u" is 1 byte
            // handle based on number of digits (1:0..9, 2:00..99, 3:100..255)
            static inline char* i2a(char* p, U<1> u, Magnitude mag, ZFill z) {
                if (unlikely(z == NoFill && u < 10)) return out<char>(p,'0'+u);
                if (mag == NotLarge || likely(u < 100)) return out(p,dd(u));
                return ( D==Fwd
                         ?(  out(out<char>(p,'0'+u/100),dd(u%100)))
                         :(  out<char>(out(p,dd(u%100)),'0'+u/100)));
            }

            // i2a: handle unsigned integral values (selected by SFINAE)
            template<typename U,
                std::enable_if_t<not std::is_signed<U>::value
                                 && std::is_integral<U>::value>* = nullptr>
            static inline char* i2a( U u, char* p )
            {
                return convert<D>::template i2a(p,u,MaybeLarge,NoFill);
            }

            // i2a: handle signed integral values (selected by SFINAE)
            template<typename I, size_t N=sizeof(I),
                std::enable_if_t<std::is_signed<I>::value
                                 && std::is_integral<I>::value>* = nullptr>
            static inline char* i2a( I i, char* p )
            {
                // Need "mask" to be filled with a copy of the sign bit.
                // If "i" is a negative value, then the result of "operator >>"
                // is implementation-defined, though usually it is an arithmetic
                // right shift that replicates the sign bit.
                // We use a conditional expression to be portable,
                // a good optimizing compiler generates an arithmetic right shift
                // and avoids the conditional branch.
                U<N> mask = i < 0 ? ~U<N>(0) : 0;
                // Now get the absolute value of "i" and cast to unsigned type U<N>.
                // Cannot use std::abs() because the result is undefined
                // in 2's complement systems for the most-negative value.
                // Want to avoid conditional branch for performance reasons since
                // CPU branch prediction will be ineffective when negative values
                // occur randomly.
                // Let "u" be "i" cast to unsigned type U<N>.
                // Subtract "u" from 2*u if "i" is positive or 0 if "i" is negative.
                // This yields the absolute value with the desired type without
                // using a conditional branch and without invoking undefined or
                // implementation defined behavior:
                U<N> u = ((2 * U<N>(i)) & ~mask) - U<N>(i);
                // We unconditionally store a minus sign when producing digits
                // in a forward direction and increment the pointer only if
                // the value was in fact negative.
                // This avoids a conditional branch and is safe because we will
                // always produce at least one digit and it will overwrite the
                // minus sign when the value is not negative.
                if (D == Fwd) { *p = '-'; p += (mask&1); }
                p = convert<D>::template i2a(p,u,MaybeLarge,NoFill);
                if (D == Rev && mask) *--p = '-';
                return p;
            }
        };

    // These are instantiated for both forward and reverse directions
    extern template char* convert<Fwd>::i2a(char*, U<2>, Magnitude, ZFill );
    extern template char* convert<Rev>::i2a(char*, U<2>, Magnitude, ZFill );
    extern template char* convert<Fwd>::i2a(char*, U<4>, Magnitude, ZFill );
    extern template char* convert<Rev>::i2a(char*, U<4>, Magnitude, ZFill );
    extern template char* convert<Fwd>::i2a(char*, U<8>, Magnitude, ZFill );
    extern template char* convert<Rev>::i2a(char*, U<8>, Magnitude, ZFill );

} // namespace dec_

// Programming interface: itoa_fwd, itoa_rev
template<typename I> char* itoa_fwd (I i,char *p) {
     return dec_::convert<dec_::Fwd>::i2a(i,p);
}

template<typename I> char* itoa_rev (I i,char *p) {
    return dec_::convert<dec_::Rev>::i2a(i,p);
}

#endif // DEC_ITOA_IMPL_H
