/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
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

HATN_COMMON_NAMESPACE_BEGIN

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

    /**
     * @brief Get bitmask where all features are set
     * @return Bitmask with all features enabled
     */
    constexpr static Features allFeatures() noexcept
    {
        Features mask=0;
        for (size_t i=0;i<static_cast<size_t>(Feature::END);i++)
        {
            mask|=(1<<i);
        }
        return mask;
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
