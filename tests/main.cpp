#include "google/protobuf/service.h"
#include <atomic>
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
struct pack_unpack {
    using packer_type = PackerT;
    template <typename ItrT>
    static std::string pack(ItrT begin, ItrT end)
    {
        std::string result = packer::pack(message.size());
        result.append(message.begin(), message.end());
        return result;
    }
};

template <typename PackerT>
class collector {

public:
    std::size_t write(const std::uint8_t *data, std::size_t len)
    {
        const auto check = [this](auto next) {
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

using namespace srpc::common;

class string_to_int : public layer<std::string, int> {
public:
    void read_upper(std::string data) override
    {
        write_down(std::atol(data.c_str()));
    }
    void read_lower(int data) override {}
};

class mul_int : public layer<int, int> {
    int f_;

public:
    mul_int(int f)
        : f_(f) {};
    void read_upper(int data) override
    {
        write_down(data * f_);
    }
    void read_lower(int data) override {}
};

class int_to_string : public layer<int, std::string> {
public:
    void read_upper(int data) override
    {
        // std::cout << data << "\n";
        write_down(std::to_string(data));
    }
    void read_lower(std::string data) override {}
};

class printer : public layer<std::string, std::string> {
public:
    void read_upper(std::string data) override
    {
        std::cout << "'" << data << "'\n";
        if (has_lower()) {
            write_down(std::move(data));
        }
    }
    void read_lower(std::string data) override
    {
        // std::cout << "'" << data << "'\n";
        // write_down(std::to_string(data));
    }
};


namespace utils {
#define LAYER_UTILS_ENABLE_IF(cond) std::enable_if_t<(cond), bool> = true

template <int Id, typename T, LAYER_UTILS_ENABLE_IF(Id == 0)>
void connect_tuple_impl(T &value)
{
}

template <int Id, typename T, LAYER_UTILS_ENABLE_IF(Id > 0)>
void connect_tuple_impl(T &value)
{
    std::get<Id - 1>(value).connect(std::get<Id>(value));
    connect_tuple_impl<Id - 1>(value);
}

template <typename... Args>
void connect_tuple(std::tuple<Args...> &value)
{
    connect_tuple_impl<sizeof...(Args) - 1>(value);
}

#undef LAYER_UTILS_ENABLE_IF
}

//: public layer<
//      typename std::tuple_element_t<0, std::tuple<Args...>>::upper_type,
//      typename std::tuple_element_t<sizeof...(Args) - 1,
//                                    std::tuple<Args...>>::lower_type>
template <typename... Args>
class layer_list {

public:
    static const std::size_t last_index = sizeof...(Args) - 1;

    using first_element = std::tuple_element_t<0, std::tuple<Args...>>;
    using last_element = std::tuple_element_t<last_index, std::tuple<Args...>>;

    using upper_layer = layer<typename first_element::upper_type,
                              typename first_element::lower_type>;

    using lower_layer = layer<typename last_element::upper_type,
                              typename last_element::lower_type>;

    using upper_type = typename upper_layer::upper_type;
    using lower_type = typename lower_layer::lower_type;

private:
    struct upper_slot_impl : public slot<upper_type> {
        upper_slot_impl() = default;
        upper_slot_impl(upper_slot_impl &&) {};
        upper_slot_impl &operator=(upper_slot_impl &&)
        {
            return *this;
        }
        void write(upper_type message)
        {
            parent_->translate_upper(std::move(message));
        }
        layer_list *parent_ = nullptr;
    };

    struct lower_slot_impl : public slot<lower_type> {
        lower_slot_impl() = default;
        lower_slot_impl(lower_slot_impl &&) {};
        lower_slot_impl &operator=(lower_slot_impl &&)
        {
            return *this;
        }
        void write(lower_type message)
        {
            parent_->translate_lower(std::move(message));
        }
        layer_list *parent_ = nullptr;
    };

    void translate_upper(upper_type msg)
    {
        on_upper_ready_(std::move(msg));
    }

    void translate_lower(lower_type msg)
    {
        on_lower_ready_(std::move(msg));
    }

public:
    layer_list(const layer_list &) = delete;
    layer_list &operator=(const layer_list &) = delete;
    layer_list() = delete;

    layer_list(std::tuple<Args...> layers)
        : layers_(std::move(layers))
    {
        connect_self();
    }

    layer_list(layer_list &&other)
        : layers_(std::move(other.layers_))
    {
        connect_self();
        other.connect_self();
        on_lower_ready_ = std::move(other.on_lower_ready_);
        on_upper_ready_ = std::move(other.on_upper_ready_);
    }

    layer_list &operator=(layer_list &&other)
    {
        layers_ = std::move(other.layers_);
        connect_self();
        other.connect_self();
        on_lower_ready_ = std::move(other.on_lower_ready_);
        on_upper_ready_ = std::move(other.on_upper_ready_);
        return *this;
    }

    void write_upper(upper_type msg)
    {
        get<0>().upper_slot().write(std::move(msg));
    }

    void write_lower(upper_type msg)
    {
        get<last_index>().lower_slot().write(std::move(msg));
    }

    template <int Id>
    auto &get()
    {
        return std::get<Id>(layers_);
    }

    void on_upper_ready_connect(std::function<void(upper_type)> val)
    {
        on_upper_ready_ = std::move(val);
    }

    void on_lower_ready_connect(std::function<void(lower_type)> val)
    {
        on_lower_ready_ = std::move(val);
    }

private:
    void connect_self()
    {
        utils::connect_tuple(layers_);
        upper_connector_.parent_ = this;
        lower_connector_.parent_ = this;
        get<0>().set_upper(&upper_connector_);
        get<last_index>().set_lower(&lower_connector_);
    }

    upper_slot_impl upper_connector_;
    lower_slot_impl lower_connector_;
    std::tuple<Args...> layers_;
    std::function<void(upper_type)> on_upper_ready_ = [](upper_type) {};
    std::function<void(lower_type)> on_lower_ready_ = [](lower_type) {};
};

template <typename... Args>
auto make_layer_tuple(Args &&... args)
{
    return layer_list<Args...>(std::make_tuple(std::forward<Args>(args)...));
}


int main()
{
    string_to_int sti;
    mul_int mi { 2 };
    int_to_string its;
    printer prt;

    auto sss
        = make_layer_tuple(std::move(sti), std::move(mi), std::move(its),
                           std::move(prt), printer {}, printer {}, printer {});

    // sss.upper_slot().write("1123");

    auto bbb = std::move(sss);

    bbb.on_lower_ready_connect([](auto msg) {
        std::cout << "Hello from slot!\n";
        std::cout << msg << "\n";
    });

    bbb.get<1>() = std::move(mul_int { 10 });
    bbb.write_upper("1123");

    // auto ll = make_layer_list(std::move(sti), std::move(mi),
    // std::move(its),
    //                          std::move(prt));

    //// its.connect(ll);
    // ll.upper_slot().write("123");
    //// its.upper_slot().write(123);

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
