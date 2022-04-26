#pragma once
// copied from golang "shinnytech.com/easyfuture/lib/utility/fastbuffer.go"
#include <cstddef>
#include <algorithm>
#include <string>
#include <memory>

namespace structlog
{

class FastBufferGuard;

class FastBuffer
{
public:
    FastBuffer() : r_(0), cap_(0), end_(nullptr) {}
    FastBuffer(const FastBuffer& b) : r_(b.end_ - b.get()), cap_(r_), b_(new char[r_]), end_(std::copy_n(b.get(), r_, b_.get())) {}
    // non null terminated
    const char* get() const
    {
        return b_.get();
    }
    std::size_t size()
    {
        return static_cast<std::size_t>(end_ - get());
    }
    // remove n char at the end
    void shrink(std::size_t n)
    {
        end_ -= n;
        r_ -= n;
    }
private:
    std::size_t r_;
    std::size_t cap_;
    std::unique_ptr<char[]> b_;
    char* end_;
    friend class FastBufferGuard;
};

class FastBufferGuard
{
public:
    FastBufferGuard(FastBuffer& fb, std::size_t n) : fb_(fb), n_(0)
    {
        reserve(n);
    }
    ~FastBufferGuard()
    {
        fb_.r_ -= n_;
    }
    std::size_t remain()
    {
        return n_;
    }
    // valid until next modification call
    char* data()
    {
        return fb_.end_;
    }
    void consume(std::size_t n)
    {
        fb_.end_ += n;
        n_ -= n;
    }
    void reserve(std::size_t n)
    {
        n_ += n;
        fb_.r_ += n;
        if (fb_.r_ > fb_.cap_) {
            auto size = static_cast<std::size_t>(fb_.end_ - fb_.get());
            fb_.cap_ = fb_.r_ * 2;
            std::unique_ptr<char[]> nb(new char[fb_.cap_]);
            if (!size)
                fb_.end_ = nb.get();
            else
                fb_.end_ = std::copy_n(fb_.get(), size, nb.get());
            fb_.b_.swap(nb);
        }
    }
    void append(const char c)
    {
        *fb_.end_++ = c;
        n_--;
    }
    void append(const std::string& str)
    {
        append(str.c_str(), str.size());
    }
    void append(const char* s, std::size_t n)
    {
        fb_.end_ = std::copy_n(s, n, fb_.end_);
        n_ -= n;
    }
    void append(const char* s)
    {
        // @todo: optimize for vectorization(something like strcpy-avx2.S)
        char* dst = fb_.end_;
        while (*s)
            *dst++ = *s++;
        n_ -= dst - fb_.end_;
        fb_.end_ = dst;
    }
    template <std::size_t N>
    void append(const char(&s)[N])
    {
        append(s, N - 1);
    }

private:
    FastBuffer& fb_;
    std::size_t n_;
};

}  // namespace structlog
