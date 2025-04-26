/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/aclmodels.h
  */

/****************************************************************************/

#ifndef HATNUTILITYOPERATION_H
#define HATNUTILITYOPERATION_H

#include <hatn/common/translate.h>

#include <hatn/utility/utility.h>
#include <hatn/utility/accesstype.h>

HATN_UTILITY_NAMESPACE_BEGIN

constexpr const size_t OperationNameLength=64;

class OperarionFamily
{
    public:

        const std::string& familyName() const
        {
            return m_name;
        }

    protected:

        OperarionFamily(std::string name) : m_name(std::move(name))
        {}

    private:

        std::string m_name;
};

class HATN_UTILITY_EXPORT Operation
{
    public:

        Operation(const OperarionFamily* opFamily,
                  std::string name,
                  AccessMask accessMask
              ) : m_opFamily(opFamily),
                  m_name(std::move(name)),
                  m_accessMask(accessMask)
        {}

        virtual ~Operation();

        Operation(const Operation&)=default;
        Operation(Operation&&)=default;
        Operation& operator =(const Operation&)=default;
        Operation& operator =(Operation&&)=default;

        Operation(
                std::string name,
                AccessMask accessMask
            ) : m_name(std::string(name)),
                m_accessMask(accessMask)
        {}

        virtual std::string description(const common::Translator* translator) const;

        const std::string& name() const
        {
            return m_name;
        }

        AccessMask accessMask() const noexcept
        {
            return m_accessMask;
        }

        const OperarionFamily* opFamily() const
        {
            return m_opFamily;
        }

    private:

        const OperarionFamily* m_opFamily;

        std::string m_name;
        AccessMask m_accessMask;
};

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYOPERATION_H
