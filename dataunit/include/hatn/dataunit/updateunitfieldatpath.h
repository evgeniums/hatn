/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/updateunitfieldatpath.h
  *
  *  Defines helpers for updating data unit fields at certain path.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITUPDATEUNITFIELDATPATH_H
#define HATNDATAUNITUPDATEUNITFIELDATPATH_H

#include <hatn/validator/get_member.hpp>

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/field.h>
#include <hatn/dataunit/fields/bytes.h>
#include <hatn/dataunit/fields/repeated.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

namespace hana=boost::hana;

struct UnitFieldUpdater
{    
    template <typename UnitT, typename PathT>
    static auto fieldAtPath(UnitT&& unit, PathT&& path) -> decltype(auto)
    {
        return hana::fold(
            path.path(),
            std::forward<UnitT>(unit),
            [](auto&& parent, auto&& key) -> decltype(auto)
            {
                return parent.field(HATN_VALIDATOR_NAMESPACE::unwrap_object(std::forward<decltype(key)>(key)));
            }
        );
    }

    template <typename UnitT, typename PathT, typename ...Args>
    static void set(UnitT&& unit, PathT&& path, Args&&... args)
    {
        auto& field=fieldAtPath(std::forward<UnitT>(unit),std::forward<PathT>(path));
        using fieldT=std::decay_t<decltype(field)>;

        hana::if_(
            std::is_base_of<Field,fieldT>{},
            [](auto&& field, auto&&, auto&&, auto&&... args)
            {
                field.set(std::forward<decltype(args)>(args)...);
            },
            [](auto&&, auto&& unit, auto&& path, auto&&... args)
            {
                // repeated field is a parent of givent element path
                fieldAtPath(std::forward<decltype(unit)>(unit),path.parent()).set(path.key(),std::forward<decltype(args)>(args)...);
            }
        )(field,std::forward<UnitT>(unit),std::forward<PathT>(path),std::forward<Args>(args)...);
    }

    template <typename UnitT, typename PathT>
    static void unset(UnitT&& unit, PathT&& path)
    {
        fieldAtPath(std::forward<UnitT>(unit),std::forward<PathT>(path)).fieldReset();
    }

    template <typename UnitT, typename PathT>
    static void clear(UnitT&& unit, PathT&& path)
    {
        fieldAtPath(std::forward<UnitT>(unit),std::forward<PathT>(path)).fieldClear();
    }

    template <typename UnitT, typename PathT>
    static void resize(UnitT&& unit, PathT&& path, size_t size)
    {
        auto& field=fieldAtPath(std::forward<UnitT>(unit),std::forward<PathT>(path));
        using fieldT=std::decay_t<decltype(field)>;

        hana::if_(
            std::is_base_of<BytesType,fieldT>{},
            [&size](auto&& field)
            {
                field.buf()->resize(size);
            },
            [&size](auto&& field)
            {
                hana::if_(
                    std::is_base_of<RepeatedType,fieldT>{},
                    [size](auto&& field)
                    {
                        field.resize(size);
                    },
                    [](auto&&)
                    {
                    }
                )(std::forward<decltype(field)>(field));
            }
        )(field);
    }

    template <typename UnitT, typename PathT>
    static void reserve(UnitT&& unit, PathT&& path, size_t size)
    {
        auto& field=fieldAtPath(std::forward<UnitT>(unit),std::forward<PathT>(path));
        using fieldT=std::decay_t<decltype(field)>;

        hana::if_(
            std::is_base_of<BytesType,fieldT>{},
            [&size](auto&& field)
            {
                field.buf()->reserve(size);
            },
            [&size](auto&& field)
            {
                hana::if_(
                    std::is_base_of<RepeatedType,fieldT>{},
                    [size](auto&& field)
                    {
                        field.reserve(size);
                    },
                    [](auto&&)
                    {
                    }
                )(std::forward<decltype(field)>(field));
            }
        )(field);
    }

    template <typename UnitT, typename PathT, typename ... Args>
    static void append(UnitT&& unit, PathT&& path, Args&&... args)
    {
        auto& field=fieldAtPath(std::forward<UnitT>(unit),std::forward<PathT>(path));
        using fieldT=std::decay_t<decltype(field)>;

        hana::if_(
            std::is_base_of<BytesType,fieldT>{},
            [](auto&& field, auto&&... args)
            {
                field.buf()->append(std::forward<decltype(args)>(args)...);
            },
            [](auto&& field, auto&&... args)
            {
                hana::if_(
                    std::is_base_of<RepeatedType,fieldT>{},
                    [](auto&& field, auto&&... args)
                    {
                        field.appendValues(std::forward<decltype(args)>(args)...);
                    },
                    [](auto&&, auto&&...)
                    {
                    }
                )(std::forward<decltype(field)>(field),std::forward<decltype(args)>(args)...);
            }
        )(field,std::forward<Args>(args)...);
    }

    template <typename UnitT, typename PathT>
    static void autoAppend(UnitT&& unit, PathT&& path, size_t size)
    {
        auto& field=fieldAtPath(std::forward<UnitT>(unit),std::forward<PathT>(path));
        using fieldT=std::decay_t<decltype(field)>;

        hana::if_(
            std::is_base_of<RepeatedType,fieldT>{},
            [&size](auto&& field)
            {
                field.addValues(size);
            },
            [](auto&&)
            {
            }
        )(field);
    }
};

struct setUnitFieldAtPathT
{
    template <typename UnitT, typename PathT, typename ...Args>
    void operator () (UnitT&& unit, PathT&& path, Args&&... args) const
    {
        UnitFieldUpdater::set(std::forward<UnitT>(unit),std::forward<PathT>(path),std::forward<Args>(args)...);
    }
};
constexpr setUnitFieldAtPathT setUnitFieldAtPath{};

struct unsetUnitFieldAtPathT
{
    template <typename UnitT, typename PathT>
    void operator () (UnitT&& unit, PathT&& path) const
    {
        UnitFieldUpdater::unset(std::forward<UnitT>(unit),std::forward<PathT>(path));
    }
};
constexpr unsetUnitFieldAtPathT unsetUnitFieldAtPath{};

struct clearUnitFieldAtPathT
{
    template <typename UnitT, typename PathT>
    void operator () (UnitT&& unit, PathT&& path) const
    {
        UnitFieldUpdater::clear(std::forward<UnitT>(unit),std::forward<PathT>(path));
    }
};
constexpr clearUnitFieldAtPathT clearUnitFieldAtPath{};

struct resizeUnitFieldAtPathT
{
    template <typename UnitT, typename PathT>
    void operator () (UnitT&& unit, PathT&& path, size_t size) const
    {
        UnitFieldUpdater::resize(std::forward<UnitT>(unit),std::forward<PathT>(path),size);
    }
};
constexpr resizeUnitFieldAtPathT resizeUnitFieldAtPath{};

struct reserveUnitFieldAtPathT
{
    template <typename UnitT, typename PathT>
    void operator () (UnitT&& unit, PathT&& path, size_t size) const
    {
        UnitFieldUpdater::reserve(std::forward<UnitT>(unit),std::forward<PathT>(path),size);
    }
};
constexpr reserveUnitFieldAtPathT reserveUnitFieldAtPath{};

struct appendUnitFieldAtPathT
{
    template <typename UnitT, typename PathT, typename ...Args>
    void operator () (UnitT&& unit, PathT&& path, Args&&... args) const
    {
        UnitFieldUpdater::append(std::forward<UnitT>(unit),std::forward<PathT>(path),std::forward<Args>(args)...);
    }
};
constexpr appendUnitFieldAtPathT appendUnitFieldAtPath{};

struct autoAppendUnitFieldAtPathT
{
    template <typename UnitT, typename PathT>
    void operator () (UnitT&& unit, PathT&& path, size_t size) const
    {
        UnitFieldUpdater::autoAppend(std::forward<UnitT>(unit),std::forward<PathT>(path),size);
    }
};
constexpr autoAppendUnitFieldAtPathT autoAppendUnitFieldAtPath{};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITUPDATEUNITFIELDATPATH_H
