#pragma once

#include <vector>
#include <algorithm>

namespace
{
    // Returns the median of a copy of the supplied values.
    float Median(std::vector<float> values)
    {
        if (values.empty())
            return 0.0f;

        const size_t middle = values.size() / 2;
        std::nth_element(values.begin(), values.begin() + middle, values.end());

        float median = values[middle];

        if (values.size() % 2 == 0)
        {
            const auto lower = std::max_element(values.begin(), values.begin() + middle);
            median = (*lower + median) * 0.5f;
        }

        return median;
    }

    // Returns the arithmetic mean over the half-open range [begin, end).
    float Mean(const std::vector<float>& values, size_t begin, size_t end)
    {
        if (begin >= end || end > values.size())
            return 0.0f;

        double sum = 0.0;

        for (size_t i = begin; i < end; ++i)
            sum += values[i];

        return static_cast<float>(sum / static_cast<double>(end - begin));
    }
}
