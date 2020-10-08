#pragma once

#include "srpc/common/layer.h"

namespace srpc { namespace common { namespace layers {
    
template <typename PackerT>
class pack_unpack : public layer<std::string, std::string> {

    using packer_type = PackerT;
    using packer_size_type = typename packer_type::size_type;

    void on_upper_data(std::string msg) override
    {
        std::string result
            = packer_type::pack(static_cast<packer_size_type>(msg.size()));
        result.append(msg.begin(), msg.end());
        write_down(std::move(result));
    }

    void on_lower_data(std::string msg) override
    {
        const auto check = [this](auto next) {
            return (std::get<0>(next) > 0)
                && collector_.size() >= (std::get<0>(next) + std::get<1>(next));
        };

        collector_.insert(collector_.end(), msg.begin(), msg.end());

        auto next = PackerT::unpack(collector_.begin(), collector_.end());
        while (check(next)) {
            collector_.erase(collector_.begin(),
                             collector_.begin() + std::get<0>(next));
            msg.assign(collector_.begin(),
                       collector_.begin() + std::get<1>(next));
            collector_.erase(collector_.begin(),
                             collector_.begin() + std::get<1>(next));

            next = PackerT::unpack(collector_.begin(), collector_.end());

            write_up(std::move(msg));
        }
    }

    std::deque<char> collector_;
};

}}}
