/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/timestretcher.h
  *
  *  Pitch-preserving time stretching of mono 48 kHz speech (WSOLA).
  *
  */

/****************************************************************************/

#ifndef HATNMEDIATIMESTRETCHER_H
#define HATNMEDIATIMESTRETCHER_H

#include <cstddef>
#include <cstdint>
#include <memory>

#include <hatn/media/media.h>
#include <hatn/media/audioformat.h>

HATN_MEDIA_NAMESPACE_BEGIN

//! Slowest playback speed, a ratio: 0.5 plays at half speed.
constexpr const float MinPlaybackSpeed=0.5f;

//! Fastest playback speed, a ratio: 2.0 plays twice as fast.
constexpr const float MaxPlaybackSpeed=2.0f;

class TimeStretcher_p;

/**
 * @brief Changes how long a piece of speech lasts without changing its pitch.
 *
 * Waveform Similarity Overlap-Add: the input is cut into overlapping 20 ms segments taken every
 * `speed * 10 ms` and laid down again every 10 ms, each segment shifted by up to +/- 6 ms to where
 * it best continues the previous one, so the pitch periods line up and there is no click or
 * warble at the joins. A speed of 2 therefore plays twice as fast and a speed of 0.5 half as
 * fast, at the original pitch. It is meant for speech in the range MinPlaybackSpeed to
 * MaxPlaybackSpeed; music and sounds far outside that range are not what it was tuned for.
 *
 * Only the input length matters: for `n` input frames the output is exactly `round(n / speed)`
 * frames long once finish() has been called, and the output does not depend on how the input was
 * cut into push() calls or the output into pull() calls.
 *
 * NOT realtime: push() and pull() allocate and run a correlation search (about 30 million
 * multiply-adds per second of audio). Call them from a decode thread, never from an audio callback.
 * Not thread-safe: one thread owns an instance. Depends on nothing but the standard library, so it
 * works without the Ogg/Opus codec.
 *
 * Latency is about 26 ms of input: the first output needs that much.
 */
class HATN_MEDIA_EXPORT TimeStretcher
{
    public:

        TimeStretcher();
        ~TimeStretcher();

        TimeStretcher(const TimeStretcher&)=delete;
        TimeStretcher(TimeStretcher&&)=delete;
        TimeStretcher& operator=(const TimeStretcher&)=delete;
        TimeStretcher& operator=(TimeStretcher&&)=delete;

        /**
         * @brief Forget everything and start a new stream at the given speed.
         * @param speed Ratio, clamped to MinPlaybackSpeed..MaxPlaybackSpeed. A NaN counts as 1.
         */
        void reset(float speed);

        //! The speed set by reset(), after clamping.
        float speed() const noexcept;

        /**
         * @brief Append mono 48 kHz input. Ignored after finish().
         *
         * Keep the pieces modest (a few thousand frames): the input is held until pull() has used it.
         */
        void push(const int16_t* pcm, size_t frames);

        //! There is no more input: pull() now also produces the last audio and then finishes.
        void finish();

        /**
         * @brief Take up to `maxFrames` of output.
         * @return Frames written to `out`. 0 means either "give me more input" (push() or finish()
         *         and try again) or, once finished() is true, "that was all".
         */
        size_t pull(int16_t* out, size_t maxFrames);

        //! finish() was called and every frame of output has been pulled.
        bool finished() const noexcept;

    private:

        std::unique_ptr<TimeStretcher_p> d;
};

HATN_MEDIA_NAMESPACE_END

#endif // HATNMEDIATIMESTRETCHER_H
