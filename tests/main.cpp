#include "google/protobuf/service.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <iostream>
#include <string_view>
#include <thread>

#include "srpc/common/layer.h"
#include "srpc/common/layer_list.h"
#include "srpc/common/packint/fixint.h"
#include "srpc/common/packint/varint.h"

using packer = srpc::common::packint::fixint<std::uint16_t>;
// using packer = srpc::common::packint::varint<std::size_t>;

using namespace srpc::common;

class string_to_int : public layer<std::string, int> {
public:
    void on_upper_data(std::string data) override
    {
        write_down(std::atol(data.c_str()));
    }
    void on_lower_data(int data) override {}
};

class mul_int : public layer<int, int> {
    int f_;

public:
    mul_int(int f)
        : f_(f) {};
    void on_upper_data(int data) override
    {
        write_down(data * f_);
    }
    void on_lower_data(int data) override {}
};

class int_to_string : public layer<int, std::string> {
public:
    void on_upper_data(int data) override
    {
        // std::cout << data << "\n";
        write_down(std::to_string(data));
    }
    void on_lower_data(std::string data) override {}
};

class printer : public layer<std::string, std::string> {
public:
    void on_upper_data(std::string data) override
    {
        std::cout << "'" << data << "'\n";
        if (has_lower()) {
            write_down(std::move(data));
        }
    }
    void on_lower_data(std::string data) override
    {
        // std::cout << "'" << data << "'\n";
        // write_down(std::to_string(data));
    }
};

using namespace srpc::common;

template <typename PackerT>
class unpacker_layer : public layer<std::string, std::string> {

    using packer_type = PackerT;
    using packer_size_type = typename packer_type::size_type;

public:
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
        std::size_t length = 0;
        auto next = PackerT::unpack(collector_.begin(), collector_.end());
        while (check(next)) {
            collector_.erase(collector_.begin(),
                             collector_.begin() + std::get<0>(next));
            std::string ready(collector_.begin(),
                              collector_.begin() + std::get<1>(next));
            collector_.erase(collector_.begin(),
                             collector_.begin() + std::get<1>(next));
            next = PackerT::unpack(collector_.begin(), collector_.end());
            write_up(std::move(ready));
        }
    }

private:
    std::deque<char> collector_;
};

int main()
{
    string_to_int sti;
    mul_int mi { 2 };
    int_to_string its;
    printer prt;

    auto sss = make_layer_list(std::move(sti), std::move(mi), std::move(its),
                               std::move(prt), printer {});

    auto bbb = make_layer_list(unpacker_layer<packer> {});

    std::string packed;

    bbb.on_lower_ready_connect([&packed](auto msg) {
        // std::cout << "ready to send: " << msg.size() << " bytes\n";
        packed = msg;
    });

    bbb.on_upper_ready_connect([&packed](auto msg) {
        // std::cout << "ready to receive: " << msg << "\n";
        packed = msg;
    });

    auto start = std::chrono::high_resolution_clock::now();
    packed = "01234567890"
             "1234567890"
             "1234567890"
             "1234567890"
             "1234567890"
             "1234567890"
             "1234567890"
             "1234567890"
             "1234567890"
             "123456789";

    auto max = 10'0000;

    for (int i = 0; i < max; ++i) {
        // bbb.write_upper("0123456789");
        bbb.write_upper(std::move(packed));
        bbb.write_lower(std::move(packed));
        // bbb.write_lower({ packed.begin(), packed.end() });
        // if (packed != "0123456789") {
        //    throw std::runtime_error("FUUUUUUUUUU!");
        //}
    }
    auto stop = std::chrono::high_resolution_clock::now();

    auto time_work
        = std::chrono::duration_cast<std::chrono::seconds>(stop - start)
              .count();

    std::cout << time_work << "\n";

    std::cout << (max * packed.size() / (time_work > 0 ? time_work : 1))
              << " bytes in second \n";
    std::cout << "packed size " << packed.size() << "\n";
    return 0;
}
