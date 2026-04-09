#include "FFT.h"

#include <algorithm>
#include <cmath>
#include <complex>

using Complex = std::complex<float>;

static size_t NextPowerOfTwo(size_t v)
{
    size_t p = 1;
    while (p < v) p <<= 1;
    return p;
}

static size_t ReverseBits(size_t x, unsigned int bits)
{
    size_t y = 0;
    for (unsigned int i = 0; i < bits; ++i)
    {
        y = (y << 1) | (x & 1);
        x >>= 1;
    }
    return y;
}

void FFT::Compute(const std::vector<float>& input, std::vector<float>& output)
{
    const size_t n0 = input.size();
    if (n0 == 0)
    {
        output.clear();
        return;
    }

    const size_t n = NextPowerOfTwo(n0);
    std::vector<Complex> a(n, Complex(0.0f, 0.0f));

    for (size_t i = 0; i < n0; ++i)
        a[i] = Complex(input[i], 0.0f);

    unsigned int bits = 0;
    while ((1u << bits) < n) ++bits;

    for (size_t i = 0; i < n; ++i)
    {
        size_t j = ReverseBits(i, bits);
        if (j > i)
            std::swap(a[i], a[j]);
    }

    constexpr float PI = 3.14159265358979323846f;

    for (size_t len = 2; len <= n; len <<= 1)
    {
        const float angle = -2.0f * PI / static_cast<float>(len);
        const Complex wlen(std::cos(angle), std::sin(angle));

        for (size_t i = 0; i < n; i += len)
        {
            Complex w(1.0f, 0.0f);

            for (size_t j = 0; j < len / 2; ++j)
            {
                const Complex u = a[i + j];
                const Complex v = a[i + j + len / 2] * w;

                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;

                w *= wlen;
            }
        }
    }

    output.resize(n / 2);
    for (size_t i = 0; i < n / 2; ++i)
        output[i] = std::abs(a[i]);
}