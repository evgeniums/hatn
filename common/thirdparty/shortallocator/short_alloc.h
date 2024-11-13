#ifndef SHORT_ALLOC_H
#define SHORT_ALLOC_H

// The MIT License (MIT)
//
// Copyright (c) 2015 Howard Hinnant
//
// Updated by Evgeny Sidorov, 2024
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <memory>

namespace salloc {

template <std::size_t N, std::size_t alignment = alignof(std::max_align_t)>
class arena
{
    alignas(alignment) char buf_[N];
    char* ptr_;

public:
    ~arena() {ptr_ = nullptr;}
    arena() noexcept : ptr_(buf_) {}
    arena(const arena&) = delete;
    arena& operator=(const arena&) = delete;

    template <std::size_t ReqAlign> char* allocate(std::size_t n);
    bool deallocate(char* p, std::size_t n) noexcept;

    static constexpr std::size_t size() noexcept {return N;}
    std::size_t used() const noexcept {return static_cast<std::size_t>(ptr_ - buf_);}
    void reset() noexcept {ptr_ = buf_;}

private:
    static
        std::size_t
        align_up(std::size_t n) noexcept
    {return (n + (alignment-1)) & ~(alignment-1);}

    bool
    pointer_in_buffer(char* p) noexcept
    {
        return std::uintptr_t(buf_) <= std::uintptr_t(p) &&
               std::uintptr_t(p) <= std::uintptr_t(buf_) + N;
    }
};

template <std::size_t N, std::size_t alignment>
template <std::size_t ReqAlign>
char*
arena<N, alignment>::allocate(std::size_t n)
{
    static_assert(ReqAlign <= alignment, "alignment is too small for this arena");
    assert(pointer_in_buffer(ptr_) && "short_alloc has outlived arena");
    auto const aligned_n = align_up(n);
    if (static_cast<decltype(aligned_n)>(buf_ + N - ptr_) >= aligned_n)
    {
        char* r = ptr_;
        ptr_ += aligned_n;
        return r;
    }

    static_assert(alignment <= alignof(std::max_align_t), "you've chosen an "
                                                          "alignment that is larger than alignof(std::max_align_t), and "
                                                          "cannot be guaranteed by normal operator new");
    return nullptr;
}

template <std::size_t N, std::size_t alignment>
bool
arena<N, alignment>::deallocate(char* p, std::size_t n) noexcept
{
    assert(pointer_in_buffer(ptr_) && "short_alloc has outlived arena");
    if (pointer_in_buffer(p))
    {
        n = align_up(n);
        if (p + n == ptr_)
            ptr_ = p;
        return true;
    }
    return false;
}

template <typename T, std::size_t N, std::size_t Align = alignof(std::max_align_t),
         typename fallback_allocator_t=std::allocator<T>>
class short_alloc
{
public:
    using value_type = T;
    static auto constexpr alignment = Align;
    static auto constexpr size = N;
    using arena_type = arena<size, alignment>;
    using fallback_alloca_type=fallback_allocator_t;

private:
    arena_type& a_;
    fallback_alloca_type fallback_alloc_;

public:

    short_alloc(const short_alloc&) = default;
    short_alloc& operator=(const short_alloc&) = delete;

    short_alloc(arena_type& a) noexcept : a_(a)
    {
        static_assert(size % alignment == 0,
                      "size N needs to be a multiple of alignment Align");
    }

    short_alloc(arena_type& a, const fallback_alloca_type& fallback_alloc) noexcept
        : a_(a),
          fallback_alloc_(fallback_alloc)
    {
        static_assert(size % alignment == 0,
                      "size N needs to be a multiple of alignment Align");
    }

    template <class U>
    short_alloc(const short_alloc<U, N, alignment, fallback_alloca_type>& a) noexcept
        : a_(a.a_),
        fallback_alloc_(a.fallback_alloc_)
    {}

    template <typename _Up> struct rebind {using other = short_alloc<_Up, N, alignment, fallback_alloca_type>;};

    T* allocate(std::size_t n)
    {
        auto p=reinterpret_cast<T*>(a_.template allocate<alignof(T)>(n*sizeof(T)));
        if (p!=nullptr)
        {
            return p;
        }
#ifdef _MSC_VER
        //! @note MSVC bug when compiler thinks that type alias is not the same as other type alias
        typename fallback_alloca_type::value_type* ptr=fallback_alloc_.allocate(n);
        return reinterpret_cast<T*>(ptr);
#else
        return fallback_alloc_.allocate(n);
#endif
    }
    void deallocate(T* p, std::size_t n) noexcept
    {
        auto done=a_.deallocate(reinterpret_cast<char*>(p), n*sizeof(T));
        if (!done)
        {
#ifdef _MSC_VER
            //! @note MSVC bug when compiler thinks that type alias is not the same as other type alias
            fallback_alloc_.deallocate(reinterpret_cast<typename fallback_alloca_type::value_type*>(p),n);
#else
            fallback_alloc_.deallocate(p,n);
#endif
        }
    }

    template <typename T1, std::size_t N1, std::size_t A1,
             typename U, std::size_t M, std::size_t A2,
             typename allocT1, typename allocT2
             >
    friend
        bool
        operator==(const short_alloc<T1, N1, A1, allocT1>& x, const short_alloc<U, M, A2, allocT2>& y) noexcept;

    template <typename T, std::size_t N, std::size_t A1, typename U, std::size_t M, std::size_t A2,
             typename allocT1, typename allocT2
             >
    friend
        bool
        operator!=(const short_alloc<T, N, A1, allocT1>& x, const short_alloc<U, M, A2, allocT2>& y) noexcept;

    template <class U, std::size_t M, std::size_t A, typename alloca_t> friend class short_alloc;
};

template <typename T, std::size_t N, std::size_t A1, typename U, std::size_t M, std::size_t A2,
         typename allocT1, typename allocT2
         >
inline
    bool
    operator==(const short_alloc<T, N, A1, allocT1>& x, const short_alloc<U, M, A2, allocT2>& y) noexcept
{
    return N == M && A1 == A2 && &x.a_ == &y.a_;
}

template <typename T, std::size_t N, std::size_t A1, typename U, std::size_t M, std::size_t A2,
         typename allocT1, typename allocT2
         >
inline
    bool
    operator!=(const short_alloc<T, N, A1, allocT1>& x, const short_alloc<U, M, A2, allocT2>& y) noexcept
{
    return !(x == y);
}

}

//! @todo Test allocation in fallback allocator.

#endif  // SHORT_ALLOC_H
