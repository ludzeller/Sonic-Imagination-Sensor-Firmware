/* imag_smoother.h
 * 
 * imagination sensor firmware
 * data smoother
 * 
 * 2021-2024 rumori
 */

#pragma once

namespace imag
{

template <typename T, size_t length>
class Smoother
{
public:
    Smoother() { reset(); }

    // add new value to smooth
    void add (T newValue)
    {
        sum -= buffer.at (index);
        buffer[index] = newValue;
        sum += newValue;

        ++index;
        index %= length;
    }

    // get current smoothed valued
    T get() const { return sum / static_cast<T> (length); }

    // clear state
    void reset()
    {
        index = 0;
        sum = static_cast<T> (0);
        buffer.fill (sum);
    }

private:
    // current index in smoothing array
    size_t index;

    // current sum of smoothing array contents
    T sum;

    // smoothing array
    std::array<T, length> buffer;
};

} // namespace imag
