/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/featureset.h
  *
  * Template to work with enum'ed feature sets
  *
  */

/****************************************************************************/

#ifndef HATNFEATURESET_H
#define HATNFEATURESET_H

#include <cstddef>
#include <initializer_list>
#include <vector>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename T>
constexpr static T fillNBits(unsigned int n) noexcept
{
    // Ensure T is an unsigned integer type
    static_assert(std::is_unsigned<T>::value, "T must be an unsigned integer type");

    // Handle the case where N is 0 or greater than or equal to the total bits
    if (n >= std::numeric_limits<T>::digits) {
        return ~static_cast<T>(0); // All bits set
    }
    if (n == 0) {
        return static_cast<T>(0); // No bits set
    }

    // Standard safe way to create a mask for the N lowest bits:
    // 1. Shift 1 to the N-th position (creating 0...010...0)
    // 2. Subtract 1 to set all the lower bits (creating 0...001...1)
    return (static_cast<T>(1) << n) - 1;
}

/**
 * @brief Template for enum'ed feature sets
 */
template <typename Traits>
struct FeatureSet
{
    using Features=typename Traits::MaskType;
    using Feature=typename Traits::Feature;

    /**
     * @brief Convert feature to bitmask
     * @param feature Feature for converting
     * @return Bitmask where bit corresponding to the feature is set to 1
     */
    constexpr static Features featureBit(Feature feature) noexcept
    {
        return 1<<static_cast<size_t>(feature);
    }

    template <typename HandlerT>
    static void invokeIfSet(Features mask, Feature feature, HandlerT handler)
    {
        if (hasFeature(mask,feature))
        {
            handler();
        }
    }

    template <typename HandlerT>
    static void eachFeature(HandlerT handler, Features mask=allFeatures())
    {
        for (size_t i=0;i<static_cast<size_t>(Feature::END);i++)
        {
            auto feature=static_cast<Feature>(i);
            if (hasFeature(mask,feature))
            {
                handler(feature);
            }
        }
    }

    static std::vector<Feature> features(Features mask)
    {
        std::vector<Feature> result;
        for (size_t i=0;i<static_cast<size_t>(Feature::END);i++)
        {
            auto feature=static_cast<Feature>(i);
            if (hasFeature(mask,feature))
            {
                result.push_back(feature);
            }
        }
        return result;
    }

    /**
     * @brief Convert list of features to bitmask
     * @param features Features for converting
     * @return Bitmask where bits corresponding to the features are set to 1
     */
    constexpr static Features featureMask(std::initializer_list<Feature> features) noexcept
    {
        Features mask=0;
        for (auto&& feature:features)
        {
            mask|=featureBit(feature);
        }
        return mask;
    }

    /**
     * @brief Check if a feature is set in bitmask
     * @param mask Bitmask to query
     * @param feature Feature to check for
     * @return Result
     */
    constexpr static bool hasFeature(Features mask, Feature feature) noexcept
    {
        return mask&featureBit(feature);
    }

    constexpr static bool hasFeature(Features mask, std::initializer_list<Feature> features) noexcept
    {
        return hasFeatures(mask,features);
    }

    constexpr static bool hasFeatures(Features mask, std::initializer_list<Feature> features) noexcept
    {
        return (mask&featureMask(features)) != 0;
    }

    /**
     * @brief Get bitmask where all features are set
     * @return Bitmask with all features enabled
     */
    constexpr static Features allFeatures() noexcept
    {
        return fillNBits<Features>(static_cast<size_t>(Feature::END));
    }

    constexpr static Features fullMask() noexcept
    {
        Features mask;
        mask=~mask;
        return mask;
    }

    constexpr static void setFeature(Features& features, Feature feature) noexcept
    {
        features|=featureBit(feature);
    }

    constexpr static void unsetFeature(Features& features, Feature feature) noexcept
    {
        features&=~featureBit(feature);
    }

    template <typename T>
    static Feature feature(T value)
    {
        return static_cast<Feature>(value);
    }

    template <typename T>
    static bool isFeature(T value, Feature f)
    {
        return feature(value)==f;
    }

    static uint32_t asInt(Feature f)
    {
        return static_cast<uint32_t>(f);
    }

    FeatureSet()=delete;
    ~FeatureSet()=delete;
    FeatureSet(const FeatureSet&)=delete;
    FeatureSet(FeatureSet&&) =delete;
    FeatureSet& operator=(const FeatureSet&)=delete;
    FeatureSet& operator=(FeatureSet&&) =delete;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNFEATURESET_H
