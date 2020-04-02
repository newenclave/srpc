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

        static bool valid_length(size_t len)
        {
            return (len >= min_length) && (len <= max_length);
        }

        template <typename IterT>
        static size_t packed_length(IterT begin, const IterT &end)
        {
            size_t length = 0x00;
            std::uint8_t last = 0x80;

            for (; (begin != end) && (last & 0x80); ++begin) {
                ++length;
                last = static_cast<std::uint8_t>(*begin);
            }
            return (last & 0x80) ? 0 : length;
        }

        template <typename IterT>
        static bool valid_packed(const IterT &begin, const IterT &end)
        {
            return valid_length(packed_length(begin, end));
        }

        static size_t result_length(size_type input)
        {
            size_t res = 0;
            while (input)
                ++res, input >>= 7;
            return res ? res : 1;
        }

        static std::string pack(size_type size)
        {
            std::string res;
            append(size, res);
            return res;
        }

        static void pack(size_type size, std::string &res)
        {
            std::string tmp;
            append(size, tmp);
            res.swap(tmp);
        }

        static void append(size_type size, std::string &res)
        {
            for (; size > 0x7F; size >>= 7) {
                res.push_back(static_cast<char>((size & 0x7F) | 0x80));
            }
            res.push_back(static_cast<char>(size));
        }

        static size_t pack(size_type size, void *result)
        {
            typedef std::uint8_t u8;
            size_t index = 0;

            u8 *res = reinterpret_cast<u8 *>(result);

            for (; size > 0x7F; size >>= 7) {
                res[index++] = (static_cast<u8>((size & 0x7F) | 0x80));
            }
            res[index++] = (static_cast<u8>(size));
            return index;
        }

        template <typename IterT>
        static size_type unpack(IterT begin, const IterT &end)
        {
            size_type res = 0x00;
            std::uint32_t shift = 0x00;
            std::uint8_t last = 0x80;

            for (; begin != end; ++begin, shift += 7) {
                last = (*begin);
                res |= (static_cast<size_type>(last & 0x7F) << shift);
                if ((last & 0x80) == 0) {
                    return res;
                }
            }
            return res;
        }

        static size_t unpack(const void *data, size_t len, size_type *res)
        {
            const std::uint8_t *d = static_cast<const std::uint8_t *>(data);

            size_type res_ = 0x00;
            std::uint32_t shift = 0x00;
            std::uint8_t last = 0x80;
            size_t i = 0;

            for (; i < len; shift += 7, ++i) {
                last = d[i];
                res_ |= (static_cast<size_type>(last & 0x7F) << shift);
                if ((last & 0x80) == 0) {
                    *res = res_;
                    return i + 1;
                }
            }
            return 0;
        }

        static size_t unpack(const void *data, size_t len)
        {
            const std::uint8_t *d = static_cast<const std::uint8_t *>(data);

            size_type res_ = 0x00;
            std::uint32_t shift = 0x00;
            std::uint8_t last = 0x80;
            size_t i = 0;

            for (; i < len; shift += 7, ++i) {
                last = d[i];
                res_ |= (static_cast<size_type>(last & 0x7F) << shift);
                if ((last & 0x80) == 0) {
                    return i + 1;
                }
            }
            return 0;
        }
    };

}}}