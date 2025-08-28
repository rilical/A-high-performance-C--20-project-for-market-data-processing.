#include <cstdint>
#include <cstring>

#include <benchmark/benchmark.h>

#if __has_include("../generated/cboe_boe_v3/messages.hpp")
#include "../generated/cboe_boe_v3/messages.hpp"
#include "../generated/cboe_boe_v3/encoder.hpp"
#include "../generated/cboe_boe_v3/decoder.hpp"
#endif

#if __has_include("../generated/nasdaq_itch_5/messages.hpp")
#include "../generated/nasdaq_itch_5/messages.hpp"
#include "../generated/nasdaq_itch_5/encoder.hpp"
#include "../generated/nasdaq_itch_5/decoder.hpp"
#endif

#if __has_include("../generated/cboe_boe_v3/messages.hpp")
static void BM_BOE_LoginRequest_Encode(benchmark::State& state) {
    using namespace cboe::boe::v3;
    LoginRequest msg{};
    std::memcpy(msg.Username.data(), "TEST", 4);
    std::memcpy(msg.Password.data(), "PASSWORD123456789012", 20);
    msg.MessageType = MessageType::LoginRequest;
    uint8_t buf[64]{}; size_t written = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(Encoder::encode(msg, buf, sizeof(buf), written));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_BOE_LoginRequest_Encode);

static void BM_BOE_LoginRequest_Decode(benchmark::State& state) {
    using namespace cboe::boe::v3;
    LoginRequest msg{}; uint8_t buf[64]{}; size_t written = 0;
    std::memcpy(msg.Username.data(), "TEST", 4);
    std::memcpy(msg.Password.data(), "PASSWORD123456789012", 20);
    msg.MessageType = MessageType::LoginRequest;
    Encoder::encode(msg, buf, sizeof(buf), written);
    size_t consumed = 0; LoginRequest out{};
    for (auto _ : state) {
        benchmark::DoNotOptimize(Decoder::decode(buf, written, out, consumed));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_BOE_LoginRequest_Decode);
#endif

#if __has_include("../generated/nasdaq_itch_5/messages.hpp")
static void BM_ITCH_AddOrder_Encode(benchmark::State& state) {
    using namespace nasdaq::itch::v5;
    AddOrder msg{}; msg.Type='A'; msg.Timestamp=123456; msg.OrderId=1; msg.Side='B'; msg.Shares=100; std::memset(msg.Symbol.data(), 'A', 8); msg.Price=1234;
    uint8_t buf[64]{}; size_t written = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(Encoder::encode(msg, buf, sizeof(buf), written));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ITCH_AddOrder_Encode);

static void BM_ITCH_AddOrder_Decode(benchmark::State& state) {
    using namespace nasdaq::itch::v5;
    AddOrder msg{}; msg.Type='A'; msg.Timestamp=123456; msg.OrderId=1; msg.Side='B'; msg.Shares=100; std::memset(msg.Symbol.data(), 'A', 8); msg.Price=1234;
    uint8_t buf[64]{}; size_t written = 0; Encoder::encode(msg, buf, sizeof(buf), written);
    size_t consumed = 0; AddOrder out{};
    for (auto _ : state) {
        benchmark::DoNotOptimize(Decoder::decode(buf, written, out, consumed));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ITCH_AddOrder_Decode);
#endif

BENCHMARK_MAIN();

