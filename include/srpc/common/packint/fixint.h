#pragma once
#include <typeinfo>

#include <algorithm>
#include <cstdint>
#include <string>

namespace srpc { namespace common { namespace packint {
    template <typename SizeType>
    struct fixint {
        typedef SizeType size_type;
        static const size_t max_length = sizeof(size_type);
        static const size_t min_length = sizeof(size_type);

        static_assert(std::is_unsigned<SizeType>::value,
                      "The value should be unsigned");

    private:
        template <typename IterT>
        static size_type unpack_(IterT begin, const IterT &end)
        {
            size_type res = 0x00;
            for (size_t cur = max_length; cur > 0 && begin != end;
                 --cur, ++begin) {
                res |= static_cast<size_type>(static_cast<std::uint8_t>(*begin))
                    << ((cur - 1) << 3);
            }
            return res;
        }

    public:
        static std::string pack(size_type size)
        {
            std::string res(max_length, '\0');
            for (size_t current = max_length; current > 0; --current) {
                res[current - 1] = static_cast<char>(size & 0xFF);
                size >>= 8;
            }
            return res;
        }

        static void append(size_type size, std::string &res)
        {
            size_t last = res.size();
            res.resize(last + max_length);
            for (size_t current = max_length; current > 0; --current) {
                res[last + current - 1] = static_cast<char>(size & 0xFF);
                size >>= 8;
            }
        }

        template <typename ItrT>
        static std::tuple<std::size_t, size_type> unpack(ItrT begin, ItrT end)
        {
            typedef const std::uint8_t cu8;
            if (std::distance(begin, end) >= max_length) {
                return std::make_tuple(max_length, unpack_(begin, end));
            }
            return std::make_tuple(0, 0);
        }
    };

}}}