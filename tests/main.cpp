//#include "google/protobuf/service.h"
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

#ifdef _WIN32

#include <WinSock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32")

using socklen_t = int;
#else

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#define closesocket close
#define SOCKET_ERROR -1
using SOCKET = int;
#endif


// using packer = srpc::common::packint::fixint<std::uint16_t>;
using packer = srpc::common::packint::varint<std::size_t>;

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

private:
    std::deque<char> collector_;
};

class pass_layer : public layer<std::string, std::string> {
    void on_upper_data(std::string msg)
    {
        write_down(std::move(msg));
    }
    void on_lower_data(std::string msg)
    {
        write_up(std::move(msg));
    }
};

class echo_layer : public layer<std::string, std::string> {
    void on_upper_data(std::string msg)
    {
        write_down(std::move(msg));
    }
    void on_lower_data(std::string msg)
    {
        write_down(std::move(msg));
    }
};

class hello_layer_client : public layer<std::string, std::string> {
    void on_upper_data(std::string msg) override
    {
        write_down(std::move(msg));
    }
    void on_lower_data(std::string msg) override
    {
        if (verified_ > 0) {
            collector_.append(msg);
            auto n = collector_.find('\n');
            if (n != std::string::npos) {
                std::cout << std::string { collector_.begin(),
                                           collector_.begin() + n }
                          << "\n";
                verified_--;
                if (0 == verified_) {
                    write_up({ collector_.begin() + n + 1, collector_.end() });
                } else {
                    collector_
                        = { collector_.begin() + n + 1, collector_.end() };
                }
            }

        } else {
            write_up(std::move(msg));
        }
    }
    int verified_ = 2;
    std::string collector_;
};

class hello_layer_server : public layer<std::string, std::string> {
public:
    void on_upper_data(std::string msg) override
    {
        if (verified_) {
            write_down(std::move(msg));
        }
    }

    void on_lower_data(std::string msg) override
    {
        call_(std::move(msg));
    }

    void init()
    {
        call_ = [this](std::string msg) { hello(std::move(msg)); };
        write_down("Hello there. Who are you?\r\n");
    }

    void hello(std::string msg)
    {
        collect_.append(msg);
        if (collect_.find('\n') == std::string::npos) {
            return;
        }
        auto f = collect_.find(':');
        if (f == std::string::npos) {
            write_down("try again!\r\n");
            collect_.clear();
            return;
        }
        std::string_view user { collect_.c_str(), f };
        std::string_view password { collect_.c_str() + f + 1,
                                    collect_.size() - f - 1 };

        verified_ = true;
        write_down("Welcome " + std::string { user.begin(), user.end() }
                   + "\r\n");
        collect_.clear();
        call_ = [this](std::string msg) { pass(std::move(msg)); };
    }

    void pass(std::string msg)
    {
        write_up(std::move(msg));
    };

    std::function<void(std::string)> call_
        = [this](std::string msg) { hello(std::move(msg)); };

    std::string collect_;
    bool verified_ = false;
};

template <typename Lt>
void echo_thread_server(SOCKET s, Lt client)
{
    fd_set read_ready;
    std::string buf;
    std::cout << "Echo thread begin\n";
    client.on_lower_ready_connect(
        [s](std::string msg) { send(s, msg.c_str(), msg.size(), 0); });
    client.template get<2>().init();
    while (1) {
        FD_ZERO(&read_ready);
        FD_SET(s, &read_ready);
        if (SOCKET_ERROR
            == select(s + 1, &read_ready, nullptr, nullptr, nullptr)) {
            break;
        }
        buf.resize(4096);
        if (FD_ISSET(s, &read_ready)) {
            int size = recv(s, &buf[0], buf.size(), 0);
            if (size <= 0) {
                break;
            }
            buf.resize(size);
            client.write_lower(std::move(buf));
        }
    }
    std::cout << "Echo thread end\n";
}

void start_server(std::uint16_t port)
{
    SOCKET ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in sai {};
    sai.sin_family = AF_INET;
    sai.sin_port = htons(port);
    sai.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&opt),
               sizeof(opt));
    bind(ls, reinterpret_cast<sockaddr *>(&sai), sizeof(sai));
    listen(ls, 10);
    std::vector<std::thread> thread_list;
    while (true) {
        sockaddr_in rec {};
        sai.sin_family = AF_INET;
        socklen_t addrlen = sizeof(sai);
        SOCKET s2 = accept(ls, reinterpret_cast<sockaddr *>(&rec), &addrlen);
        auto lll = make_layer_list(echo_layer {}, unpacker_layer<packer> {},
                                   hello_layer_server {});
        thread_list.emplace_back(echo_thread_server<decltype(lll)>, s2,
                                 std::move(lll));
    }

    for (auto &t : thread_list) {
        if (t.joinable()) {
            t.join();
        }
    }
}

template <typename LayerT>
void echo_client_thread(SOCKET s, LayerT &l)
{
    std::string data;
    while (1) {
        data.resize(4096);
        int len = recv(s, data.data(), data.size(), 0);
        if (len <= 0) {
            break;
        }
        data.resize(len);
        l.write_lower(std::move(data));
    }
    std::cout << "CLient thread exit\n";
}

void start_client(std::uint16_t port)
{
    SOCKET ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in sai {};
    sai.sin_family = AF_INET;
    sai.sin_port = htons(port);
    sai.sin_addr.s_addr = inet_addr("192.168.121.133");

    std::string test = "1234567890";

    auto layers = make_layer_list(pass_layer {}, unpacker_layer<packer> {},
                                  hello_layer_client {});
    layers.on_lower_ready_connect(
        [ls](auto msg) { send(ls, msg.c_str(), msg.size(), 0); });
    layers.on_upper_ready_connect([&test](auto msg) {
        if (test != msg) {
            std::cout << msg << "\n";
        }
    });

    connect(ls, reinterpret_cast<sockaddr *>(&sai), sizeof(sai));
    int sent = send(ls, "f:f\n", 4, 0);
    std::thread t(echo_client_thread<decltype(layers)>, ls, std::ref(layers));

    // std::this_thread::sleep_for(std::chrono::seconds(1));

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000000; ++i) {
        layers.write_upper(test);
    }

    shutdown(ls, 2);
    closesocket(ls);

    if (t.joinable()) {
        t.join();
    }
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(stop
                                                                       - start)
                     .count()
              << "\n";
}

int main(int arg, char *argv[])
{
#ifdef _WIN32
    WSADATA wsad {};
    if (SOCKET_ERROR == WSAStartup(0x0202, &wsad)) {
        std::cerr << "Failed to init Socket. \n";
        return 1;
    }
#else
    signal(SIGPIPE, SIG_IGN);
#endif

    if (arg == 1) {
        start_server(4949);
    } else {
        start_client(4949);
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
