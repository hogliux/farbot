#include <iostream>
#include <thread>
#include <random>

#include "NonRealtimeMutatable.hpp"
#include "clock.hpp"

struct BiquadCoeffs { float b0, b1, b2, a1, a2; };

farbot::NonRealtimeMutatable<BiquadCoeffs> sharedCoeffs (BiquadCoeffs {1.0f, 0.0f, 0.f, 0.f, 0.f});

void processAudio(float* buffer, std::size_t n)
{
    static float lv1 = 0.f, lv2 = 0.f;

    farbot::NonRealtimeMutatable<BiquadCoeffs>::ScopedAccess<true> coeffs (sharedCoeffs);
    spin_for_duration (100000ULL);

    for (size_t i = 0; i < n; ++i)
    {
        auto input = buffer[i];
        auto output = (input * coeffs->b0) + lv1;
        buffer[i] = output;

        lv1 = (input * coeffs->b1) - (output* coeffs->a1) + lv2;
        lv2 = (input * coeffs->b2) - (output* coeffs->a2);
    }
}

void realtimeThreadEntry()
{
    constexpr std::size_t n = 512;
    std::array<float, n> buffer;

    std::fill (buffer.begin(), buffer.end(), 0.0);

    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(-1.f, 1.f);

    while (true)
    {
        for (std::size_t i = 0; i < n; ++i)
            buffer[i] = distribution(generator);
        
        processAudio (buffer.data(), n);
    }
}

int main()
{
    std::thread thread ([] () {realtimeThreadEntry();});

    spin_for_duration (1000000ULL);

    {
        farbot::NonRealtimeMutatable<BiquadCoeffs>::ScopedAccess<false> coeffs (sharedCoeffs);
        coeffs->b0 = 0.5f;
        coeffs->b1 = 0.5f;
    }

    while (true)
    {
       // spin_for_duration (1000000ULL);

        farbot::NonRealtimeMutatable<BiquadCoeffs>::ScopedAccess<false> coeffs (sharedCoeffs);
        coeffs->b0 *= 0.5f;
        coeffs->b1 *= 0.5f;
    }

    return 0;
}
