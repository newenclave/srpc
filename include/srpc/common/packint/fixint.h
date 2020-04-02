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

        static bool valid_length(size_t len)
        {
            return (len >= min_length) && (len <= max_length);
        }

        template <typename IterT>
        static size_t packed_length(IterT begin, const IterT &end)
        {
            return std::distance(begin, end) < max_length ? 0 : max_length;
        }

        template <typename IterT>
        static bool valid_packed(const IterT &begin, const IterT &end)
        {
            return valid_length(packed_length(begin, end));
        }

        static size_t result_length(size_type)
        {
            return max_length;
        }

        static std::string pack(size_type size)
        {
            std::string res(max_length, '\0');
            for (size_t current = max_length; current > 0; --current) {
                res[current - 1] = static_cast<char>(size & 0xFF);
                size >>= 8;
            }
            return res;
        }

        static void pack(size_type size, std::string &res)
        {
            char tmp[max_length];
            for (size_t current = max_length; current > 0; --current) {
                tmp[current - 1] = static_cast<char>(size & 0xFF);
                size >>= 8;
            }
            res.assign(&tmp[0], &tmp[max_length]);
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

        static size_t pack(size_type size, void *result)
        {
            std::uint8_t *res = reinterpret_cast<std::uint8_t *>(result);
            for (size_t current = max_length; current > 0; --current) {
                res[current - 1] = static_cast<std::uint8_t>(size & 0xFF);
                size >>= 8;
            }
            return max_length;
        }

        template <typename IterT>
        static size_type unpack(IterT begin, const IterT &end)
        {
            size_type res = 0x00;
            for (size_t cur = max_length; cur > 0 && begin != end;
                 --cur, ++begin) {
                res |= static_cast<size_type>(static_cast<std::uint8_t>(*begin))
                    << ((cur - 1) << 3);
            }
            return res;
        }

        static size_t unpack(const void *data, size_t len, size_type *res)
        {
            typedef const std::uint8_t cu8;
            if (len >= max_length) {
                if (res) {
                    *res = unpack(static_cast<cu8 *>(data),
                                  static_cast<cu8 *>(data) + len);
                }
                return max_length;
            }
            return 0;
        }
    };

}}}