#include "google/protobuf/service.h"
#include <cstdint>
#include <iostream>
#include <thread>

#include "srpc/common/layer.h"
#include "srpc/common/packint/varint.h"

class test_slot : public srpc::common::layer<int, int> {

public:
    void read_upper(int message)
    {
        std::cout << "from upper " << message << "\n";
    }
    void read_lower(int message)
    {
        std::cout << "from lower " << message << "\n";
    }
};

int main()
{
    test_slot ts;
    auto tts = std::move(ts);

    ts.upper_slot().write(10);
    ts.lower_slot().write(10);
    tts.upper_slot().write(10);
    tts.lower_slot().write(10);
}
