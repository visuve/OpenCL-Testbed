#pragma once

#include <stdio.h>
#include <chrono>
#include <string>

namespace
{
    using NanoSeconds = std::chrono::nanoseconds;
    using MicroSeconds = std::chrono::microseconds;
    using MilliSeconds = std::chrono::milliseconds;

    template <typename T>
    class StopWatch
    {
    public:
        StopWatch();

        virtual ~StopWatch()
        {
            PrintDuration();
        }

        T Duration() const
        {
            const auto diff = std::chrono::high_resolution_clock::now() - m_start;
            return std::chrono::duration_cast<T>(diff);
        }

        void PrintDuration() const
        {
            wprintf_s(L"%I64u %ls\n", Duration().count(), m_unit.data());
        }

        StopWatch(const StopWatch&) = delete;
        StopWatch& operator = (const StopWatch&) = delete;

    private:
        const std::chrono::high_resolution_clock::time_point m_start = std::chrono::high_resolution_clock::now();
        const std::wstring m_unit;
    };

    StopWatch<NanoSeconds>::StopWatch() :
        m_unit(L"ns")
    {
    }

    StopWatch<MicroSeconds>::StopWatch() :
        m_unit(L"us")
    {
    }

    StopWatch<MilliSeconds>::StopWatch() :
        m_unit(L"ms")
    {
    }
}