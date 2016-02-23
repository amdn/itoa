#include "itoa.h"
#include <utility>

namespace dec_
{
    template < typename T, size_t N, typename Gen, size_t... Is >
    constexpr auto generate_array( Gen&& item, std::index_sequence<Is...>)
    { return std::array<T,N>{{item(Is)...}}; }

    const std::array<char,200>
    digits = generate_array<char,200>( [](size_t i) {
            return char('0' + ((i%2) ? ((i/2)%10) : ((i/2)/10)));
        }, std::make_index_sequence<200>{} );

    template char* convert<Fwd>::i2a(char*, U<2>, bool, bool );
    template char* convert<Rev>::i2a(char*, U<2>, bool, bool );
    template char* convert<Fwd>::i2a(char*, U<4>, bool, bool );
    template char* convert<Rev>::i2a(char*, U<4>, bool, bool );
    template char* convert<Fwd>::i2a(char*, U<8>, bool, bool );
    template char* convert<Rev>::i2a(char*, U<8>, bool, bool );
}
