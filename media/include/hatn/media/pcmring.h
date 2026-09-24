/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/pcmring.h
  *
  *  Lock-free single-producer/single-consumer ring buffer of 16-bit PCM samples.
  *
  */

/****************************************************************************/

#ifndef HATNMEDIAPCMRING_H
#define HATNMEDIAPCMRING_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <vector>

#include <hatn/media/media.h>

HATN_MEDIA_NAMESPACE_BEGIN

/**
 * @brief Wait-free SPSC ring buffer of int16_t samples.
 *
 * This is the seam between a realtime audio callback and a worker thread. The audio thread must
 * never block, allocate or take a lock, so it only calls write() (capture) or read() (playback)
 * on this ring, and a worker thread does the encoding/decoding and file I/O on the other end.
 *
 * Exactly ONE thread may call the producer side (write(), writable(), writeIndex()) and exactly
 * one thread the consumer side (read(), readable(), readIndex(), discardUpTo()). They may be
 * different threads or, in tests, the same one.
 *
 * Positions are monotonically increasing 64-bit counters that never wrap in practice (at 48 kHz a
 * uint64_t lasts millions of years), so "how much was ever written" is a plain number a consumer
 * can compare against -- see VoicePlayer's use of writeIndex() as a seek marker.
 */
class PcmRing
{
    public:

        /**
         * @brief Constructor.
         * @param capacityFrames Minimum capacity. Rounded up to a power of two, at least 2.
         */
        explicit PcmRing(size_t capacityFrames)
            : m_buffer(roundUpPowerOfTwo(capacityFrames)),
              m_mask(m_buffer.size()-1)
        {}

        PcmRing(const PcmRing&)=delete;
        PcmRing(PcmRing&&)=delete;
        PcmRing& operator=(const PcmRing&)=delete;
        PcmRing& operator=(PcmRing&&)=delete;

        size_t capacity() const noexcept
        {
            return m_buffer.size();
        }

        /**
         * @brief Producer: append up to `count` samples.
         * @return Number actually written, which is less than `count` only if the ring is full.
         *         The remainder is NOT written, the caller decides whether to drop it or retry.
         */
        size_t write(const int16_t* data, size_t count) noexcept
        {
            const auto w=m_write.load(std::memory_order_relaxed);
            const auto r=m_read.load(std::memory_order_acquire);
            const auto freeSpace=m_buffer.size()-static_cast<size_t>(w-r);
            const auto n=std::min(count,freeSpace);
            copyIn(w,data,n);
            m_write.store(w+n,std::memory_order_release);
            return n;
        }

        //! Producer: free space, in samples.
        size_t writable() const noexcept
        {
            const auto w=m_write.load(std::memory_order_relaxed);
            const auto r=m_read.load(std::memory_order_acquire);
            return m_buffer.size()-static_cast<size_t>(w-r);
        }

        //! Producer: total samples ever written.
        uint64_t writeIndex() const noexcept
        {
            return m_write.load(std::memory_order_relaxed);
        }

        /**
         * @brief Consumer: remove up to `count` samples into `out`.
         * @return Number actually read.
         */
        size_t read(int16_t* out, size_t count) noexcept
        {
            const auto r=m_read.load(std::memory_order_relaxed);
            const auto w=m_write.load(std::memory_order_acquire);
            const auto n=std::min(count,static_cast<size_t>(w-r));
            copyOut(r,out,n);
            m_read.store(r+n,std::memory_order_release);
            return n;
        }

        //! Consumer: samples available to read.
        size_t readable() const noexcept
        {
            const auto r=m_read.load(std::memory_order_relaxed);
            const auto w=m_write.load(std::memory_order_acquire);
            return static_cast<size_t>(w-r);
        }

        //! Consumer: total samples ever read or discarded.
        uint64_t readIndex() const noexcept
        {
            return m_read.load(std::memory_order_relaxed);
        }

        /**
         * @brief Consumer: drop everything before absolute position `index`.
         *
         * Clamped: never moves backwards and never past what has been written. This is how a
         * consumer throws away stale audio after a seek without the producer ever touching the
         * read side.
         */
        void discardUpTo(uint64_t index) noexcept
        {
            const auto r=m_read.load(std::memory_order_relaxed);
            const auto w=m_write.load(std::memory_order_acquire);
            const auto target=std::min(std::max(index,r),w);
            m_read.store(target,std::memory_order_release);
        }

    private:

        static size_t roundUpPowerOfTwo(size_t value) noexcept
        {
            size_t result=2;
            while (result<value)
            {
                result<<=1;
            }
            return result;
        }

        void copyIn(uint64_t position, const int16_t* data, size_t count) noexcept
        {
            const auto start=static_cast<size_t>(position&m_mask);
            const auto first=std::min(count,m_buffer.size()-start);
            if (first!=0)
            {
                std::memcpy(m_buffer.data()+start,data,first*sizeof(int16_t));
            }
            if (count>first)
            {
                std::memcpy(m_buffer.data(),data+first,(count-first)*sizeof(int16_t));
            }
        }

        void copyOut(uint64_t position, int16_t* out, size_t count) const noexcept
        {
            const auto start=static_cast<size_t>(position&m_mask);
            const auto first=std::min(count,m_buffer.size()-start);
            if (first!=0)
            {
                std::memcpy(out,m_buffer.data()+start,first*sizeof(int16_t));
            }
            if (count>first)
            {
                std::memcpy(out+first,m_buffer.data(),(count-first)*sizeof(int16_t));
            }
        }

        std::vector<int16_t> m_buffer;
        size_t m_mask;

        // Own cache line each, so the producer and consumer do not false-share.
        alignas(64) std::atomic<uint64_t> m_write{0};
        alignas(64) std::atomic<uint64_t> m_read{0};
};

HATN_MEDIA_NAMESPACE_END

#endif // HATNMEDIAPCMRING_H
