#include "google/protobuf/service.h"
#include <cstdint>
#include <deque>
#include <iostream>
#include <thread>

#include "srpc/common/layer.h"
#include "srpc/common/packint/fixint.h"
#include "srpc/common/packint/varint.h"


template <typename PackerT>
std::string pack_message(const std::string &message)
{
    using packer = PackerT;
    std::string result = packer::pack(message.size());
    result.append(message.begin(), message.end());
    return result;
}

template <typename PackerT>
class collector {

public:
    std::size_t write(const std::uint8_t *data, std::size_t len)
    {
        auto check = [this](auto next) {
            return (std::get<0>(next) > 0)
                && collector_.size() >= (std::get<0>(next) + std::get<1>(next));
        };

        collector_.insert(collector_.end(), data, data + len);
        std::size_t length = 0;
        auto next = PackerT::unpack(collector_.begin(), collector_.end());
        while (check(next)) {
            collector_.erase(collector_.begin(),
                             collector_.begin() + std::get<0>(next));
            ready_.emplace_back(collector_.begin(),
                                collector_.begin() + std::get<1>(next));
            collector_.erase(collector_.begin(),
                             collector_.begin() + std::get<1>(next));
            next = PackerT::unpack(collector_.begin(), collector_.end());
        }
        return ready_.size();
    }

private:
    std::deque<std::uint8_t> collector_;
    std::deque<std::string> ready_;
};

using packer = srpc::common::packint::fixint<std::size_t>;
// using packer = srpc::common::packint::varint<std::size_t>;

int main()
{
    std::string data;
    collector<packer> collector;
    data += pack_message<packer>("123");
    data += pack_message<packer>("hello");
    data += pack_message<packer>("world!");
    data += pack_message<packer>(std::string(500, '!'));

    std::cout << collector.write((const uint8_t *)data.c_str(), data.size())
              << "\n";

    return 0;
}
