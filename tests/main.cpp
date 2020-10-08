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
#include "srpc/common/layers/pack_unpack.h"

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
using unpacker_layer = layers::pack_unpack<PackerT>;

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
        break;
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

    connect(ls, reinterpret_cast<sockaddr *>(&sai), sizeof(sai));
    int sent = send(ls, "f:f\n", 4, 0);
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000000; ++i) {
        //layers.write_upper(test);
    }

    shutdown(ls, 2);
    closesocket(ls);

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
