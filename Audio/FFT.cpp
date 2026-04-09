#include "FFT.h"
#include <complex>
#include <cmath>
#include <vector>

typedef std::complex<float> Complex;

void FFT::Compute(const std::vector<float>& input, std::vector<float>& output)
{
    int N = input.size();
    output.resize(N / 2);

    for (int k = 0; k < N / 2; k++)
    {
        Complex sum(0, 0);

        for (int n = 0; n < N; n++)
        {
            float angle = -2.0f * M_PI * k * n / N;
            sum += input[n] * Complex(cos(angle), sin(angle));
        }

        output[k] = std::abs(sum);
    }
} 