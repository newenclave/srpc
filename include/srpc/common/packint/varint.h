#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <typeinfo>

namespace srpc { namespace common { namespace packint {

    template <typename SizeType>
    struct varint {

        static_assert(std::is_unsigned<SizeType>::value,
                      "The value should be unsigned");
        typedef SizeType size_type;

        static const size_t max_length = (sizeof(size_type) * 8) / 7 + 1;
        static const size_t min_length = 1;

        static std::string pack(size_type size)
        {
            std::string res;
            append(size, res);
            return res;
        }

        static void append(size_type size, std::string &res)
        {
            for (; size > 0x7F; size >>= 7) {
                res.push_back(static_cast<char>((size & 0x7F) | 0x80));
            }
            res.push_back(static_cast<char>(size));
        }

        template <typename ItrT>
        static std::tuple<std::size_t, size_type> unpack(ItrT begin, ItrT end)
        {
            size_type res_ = 0x00;
            std::uint32_t shift = 0x00;
            std::uint8_t last = 0x80;
            size_t i = 0;

            for (; begin != end; shift += 7, ++begin, ++i) {
                last = static_cast<std::uint8_t>(*begin);
                res_ |= (static_cast<size_type>(last & 0x7F) << shift);
                if ((last & 0x80) == 0) {
                    return std::make_tuple(i + 1, res_);
                }
            }
            return std::make_tuple(0, 0);
        }
    };

}}}