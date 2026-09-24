/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/waveformextractor.h
  *
  *  Reduces PCM audio to the fixed-size waveform stored in a voice message.
  *
  */

/****************************************************************************/

#ifndef HATNMEDIAWAVEFORMEXTRACTOR_H
#define HATNMEDIAWAVEFORMEXTRACTOR_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include <hatn/media/media.h>

HATN_MEDIA_NAMESPACE_BEGIN

/**
 * @brief Computes the waveform of a voice message: WaveformBuckets bytes, 0..255 each.
 *
 * The result travels in the message metadata and is drawn by the receiver before the audio has
 * even downloaded, so the receiver never decodes the stream to draw it. It is computed ONCE at
 * record time and stored, never recomputed -- which makes every constant here part of the
 * message format: changing WaveformBuckets, FloorDb or the mapping later would make old and new
 * messages render differently and cannot be undone for messages already sent.
 *
 * The scale is logarithmic. Linear peak makes ordinary speech look almost flat next to a single
 * loud click, so each bucket is the RMS level of its stretch of audio in dBFS, mapped from
 * [FloorDb, 0] dBFS onto 0..255. Silence (and anything below FloorDb) is 0, full scale is 255.
 *
 * The audio is first reduced to one mean-square value per fixed WindowFrames window, and only
 * then resampled to WaveformBuckets buckets at finalization. That is what lets the bucket count
 * be fixed while the duration is not known until recording stops. It also makes the result
 * independent of how the caller chunks its add() calls.
 *
 * Not thread-safe; one thread feeds it. add() may allocate, so keep it off the realtime audio
 * thread (VoiceRecorder calls it from its worker).
 */
class HATN_MEDIA_EXPORT WaveformExtractor
{
    public:

        //! Number of bytes in a finished waveform. Part of the message format, see above.
        constexpr static const size_t WaveformBuckets=100;

        //! Level mapped to 0. Anything quieter is also 0. Part of the message format, see above.
        constexpr static const int FloorDb=-60;

        //! PCM frames reduced to one value before bucketing (5 ms at 48 kHz).
        constexpr static const size_t WindowFrames=240;

        WaveformExtractor()=default;

        //! Forget everything accumulated so far.
        void reset();

        //! Feed mono 16-bit PCM. Any chunk size, including sizes that split a window.
        void add(const int16_t* samples, size_t frames);

        //! Total frames fed since the last reset.
        uint64_t totalFrames() const noexcept
        {
            return m_totalFrames;
        }

        /**
         * @brief Finished waveform of everything fed so far.
         *
         * Always exactly WaveformBuckets bytes. A trailing partial window counts as a window of
         * its own. Empty input gives all zeros. Does not modify the extractor, so it can be
         * called mid-recording for a live preview.
         */
        std::vector<uint8_t> buckets() const;

        //! One-shot: waveform of a complete PCM buffer.
        static std::vector<uint8_t> compute(const int16_t* samples, size_t frames);

        //! Map a mean-square sample value (in int16 units squared) to 0..255 on the log scale.
        static uint8_t meanSquareToByte(double meanSquare) noexcept;

        /**
         * @brief RMS level of a PCM buffer in dBFS, for a live level meter.
         * @return Value in [FloorDb, 0]; FloorDb for silence or an empty buffer.
         */
        static float rmsDb(const int16_t* samples, size_t frames) noexcept;

    private:

        std::vector<float> m_windows;   //!< mean-square of each completed window
        uint64_t m_partialSumSq=0;      //!< sum of squares of the window being filled
        size_t m_partialFrames=0;       //!< frames in the window being filled
        uint64_t m_totalFrames=0;
};

HATN_MEDIA_NAMESPACE_END

#endif // HATNMEDIAWAVEFORMEXTRACTOR_H
