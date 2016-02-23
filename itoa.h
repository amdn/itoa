
#ifndef DEC_ITOA_CONVERT_H
#define DEC_ITOA_CONVERT_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

namespace dec_ {

    using uint128_t = unsigned __int128;

    extern const std::array<char,200> digits;
    static inline uint16_t const& dd(uint8_t u) {
        return reinterpret_cast<uint16_t const*>(digits.data())[u];
    }

    enum Direction {Fwd,Rev};

    template<typename T> static constexpr T pow10(size_t x) {
        return x ? 10*pow10<T>(x-1) : 1;
    }

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

    template<size_t N> struct QR { U<N> q; U<N/2> r; };
    template<size_t N> QR<N> static inline split( U<N> u ) {
        constexpr MI<N> mi{};
        U<N> q = (mi.m * (U<2*N>(u)+mi.a)) >> mi.s;
        return { q, U<N/2>( u - q * pow10<U<N/2>>(N) ) };
    }

    template < Direction D >
        struct convert
        {
            static constexpr bool L=false, Z=true;

            template<typename T>
            static inline char* out(char* p, T&& obj) {
                if (D==Rev) p -= sizeof(T);
                memcpy(p,reinterpret_cast<const void*>(&obj),sizeof(T));
                if (D==Fwd) p += sizeof(T);
                return p;
            }

            template<typename UInt, size_t N=sizeof(UInt)>
            static inline char* i2a_small(char* p, UInt u, bool z) {
                return ( not z
                         ?( i2a(p,U<N/2>(u),L,z))
                         :( D==Fwd
                            ?( i2a( i2a(p,U<N/2>(0),L,z),U<N/2>(u),L,z))
                            :( i2a( i2a(p,U<N/2>(u),L,z),U<N/2>(0),L,z))));
            }

            template<size_t N>
            static inline char* i2a_medium(char* p, bool z, QR<N> x) {
                return ( D==Fwd
                         ?( i2a( i2a(p,U<N/2>(x.q),L,z),x.r,L,Z))
                         :( i2a( i2a(p,x.r,L,Z),U<N/2>(x.q),L,z)));
            }

            template<size_t N>
            static inline char* i2a_large(char* p, bool z, QR<N> x) {
                auto y = split<N>(x.q);
                return ( D==Fwd
                         ?( i2a( i2a( i2a(p,U<N/2>(y.q),L,z),y.r,L,Z),x.r,L,Z))
                         :( i2a( i2a( i2a(p,x.r,L,Z),y.r,L,Z),U<N/2>(y.q),L,z)));
            }

            template<typename UInt, size_t N=sizeof(UInt)>
            static inline char* i2a(char* p, UInt u, bool large, bool z) {
                if (u < pow10<U<N>>(N)) return i2a_small(p,u,z);
                auto x = split<N>(u);
                if (not large || u < pow10<U<N>>(2*N)) return i2a_medium<N>(p,z,x);
                return i2a_large<N>(p,z,x);
            }
                    
            static inline char* i2a(char* p, U<1> u, bool large, bool z) {
                if (unlikely(not z && u < 10)) return out<char>(p,'0'+u);
                if (not large || likely(u < 100)) return out(p,dd(u));
                return ( D==Fwd
                         ?(  out(out<char>(p,'0'+u/100),dd(u%100)))
                         :(  out<char>(out(p,dd(u%100)),'0'+u/100)));
            }
        };

    template<Direction D, typename I, size_t N>
    static inline char* i2a( I i, char* p )
    {
        bool msb = 1 & (i >> (8*N-1));
        bool negative = msb & std::is_signed<I>::value;
        if (likely(not negative)) {
            p = convert<D>::template i2a(p,U<N>(i),true,false);
        } else {
            if (D == Fwd) *p++ = '-';
            p = convert<D>::template i2a(p,U<N>(0)-U<N>(i),true,false);
            if (D == Rev) *--p = '-';
        }
        return p;
    }

    extern template char* convert<Fwd>::i2a(char*, U<2>, bool, bool );
    extern template char* convert<Rev>::i2a(char*, U<2>, bool, bool );
    extern template char* convert<Fwd>::i2a(char*, U<4>, bool, bool );
    extern template char* convert<Rev>::i2a(char*, U<4>, bool, bool );
    extern template char* convert<Fwd>::i2a(char*, U<8>, bool, bool );
    extern template char* convert<Rev>::i2a(char*, U<8>, bool, bool );
}

template<typename I, size_t N = sizeof(I)>
    char* itoa_fwd (I i,char *p) { return dec_::i2a<dec_::Fwd,I,N>(i,p); }

template<typename I, size_t N = sizeof(I)>
    char* itoa_rev (I i,char *p) { return dec_::i2a<dec_::Rev,I,N>(i,p); }

#endif // DEC_ITOA_CONVERT_H
