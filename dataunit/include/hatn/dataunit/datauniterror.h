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

    static RawError& threadLocal() noexcept;
    static bool isEnabledTL() noexcept;
    static void setEnabledTL(bool) noexcept;

    private:

        static bool& enablingTL() noexcept;
};

//--------------------------------------------------------------

class HATN_DATAUNIT_EXPORT UnitNativeError : public common::NativeError
{
    public:

        UnitNativeError(const RawError& rawError);

        int fieldId() const noexcept
        {
            return m_field;
        }

    private:

        int m_field;
};

//--------------------------------------------------------------

//! Error category for hatndataunit.
class HATN_DATAUNIT_EXPORT DataunitErrorCategory : public common::ErrorCategory
{
public:

    //! Name of the category
    virtual const char *name() const noexcept override
    {
        return "hatn.dataunit";
    }

    //! Get description for the code
    virtual std::string message(int code) const override;

    //! Get string representation of the code.
    virtual const char* codeString(int code) const override;

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

template <typename ...Args>
void prepareRawError(RawErrorCode code, const char* msg, Args&&... args)
{
    if (RawError::threadLocal().message.empty())
    {
        RawError::threadLocal().message=fmt::format(msg,std::forward<Args>(args)...);
        RawError::threadLocal().code=code;
    }
    else
    {
        auto newMessage=fmt::format(msg,std::forward<Args>(args)...);
        RawError::threadLocal().message=fmt::format("{}: {}",newMessage, RawError::threadLocal().message);
    }
}

template <typename ...Args>
void rawError(RawErrorCode code, int field, const char* msg, Args&&... args)
{
    if (RawError::isEnabledTL())
    {
        prepareRawError(code,msg,std::forward<Args>(args)...);
        RawError::threadLocal().field=field;
    }
}

template <typename ...Args>
void rawError(RawErrorCode code, const char* msg, Args&&... args)
{
    if (RawError::isEnabledTL())
    {
        prepareRawError(code,msg,std::forward<Args>(args)...);
    }
}

//---------------------------------------------------------------

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITERROR_H
