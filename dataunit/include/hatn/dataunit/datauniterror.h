/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/datauniterror.h
  *
  *  Declares error classes.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITERROR_H
#define HATNDATAUNITERROR_H

#include <string>

#include <hatn/common/error.h>
#include <hatn/common/nativeerror.h>

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/datauniterrorcodes.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

//---------------------------------------------------------------

struct HATN_DATAUNIT_EXPORT RawError
{
    RawErrorCode code=RawErrorCode::OK;
    std::string message;
    int field=-1;

    void reset() noexcept
    {
        code=RawErrorCode::OK;
        message.clear();
        field=-1;
    }

    operator bool() const noexcept
    {
        return code!=RawErrorCode::OK;
    }

    static RawError& threadLocal();
};

//--------------------------------------------------------------

class HATN_DATAUNIT_EXPORT UnitNativeError : public common::NativeError
{
    public:

        UnitNativeError(const RawError& rawError);

        int unitField() const noexcept
        {
            return m_field;
        }

    private:

        int m_field;
};

//--------------------------------------------------------------

//! Error category for hatndataunit.
class HATN_DATAUNIT_EXPORT DataunitErrorCategory : public std::error_category
{
public:

    //! Name of the category
    virtual const char *name() const noexcept override
    {
        return "hatn.dataunit";
    }

    //! Get description for the code
    virtual std::string message(int code) const override;

    //! Get category
    static const DataunitErrorCategory& getCategory() noexcept;
};

//---------------------------------------------------------------

inline void fillError(UnitError code, Error& ec)
{
    if (RawError::threadLocal())
    {
        auto native=std::make_shared<UnitNativeError>(RawError::threadLocal());
        ec.setNative(static_cast<int>(code),std::move(native));
    }
}

//---------------------------------------------------------------

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITERROR_H
