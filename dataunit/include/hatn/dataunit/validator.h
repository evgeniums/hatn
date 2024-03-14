/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** \file dataunit/validator.h
  *
  *  Base class for unit validators
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITBASEVALIDATOR_H
#define HATNDATAUNITBASEVALIDATOR_H

#include <type_traits>
#include <functional>

#include <hatn/common/translate.h>
#include <hatn/dataunit/field.h>
#include <hatn/dataunit/unit.h>

HATN_NAMESPACE_BEGIN
using HATN_COMMON_NS::_TR;
namespace HATN_DATAUNIT_NS {

//---------------------------------------------------------------

class Validator;
using ValidateCb=std::function<void (bool,const std::string&)>;

namespace details {
template <FieldGetSet::Operation Op>
struct ValidatorTraits
{
};
template <>
struct ValidatorTraits<FieldGetSet::Operation::Set>
{
    template <typename T>
    inline static bool validate(Validator* validator,const Field* field,const ValidateCb& validateCb,const T& val);
};
template <>
struct ValidatorTraits<FieldGetSet::Operation::ArrayResize>
{
    inline static bool validate(Validator* validator,const Field* field,const ValidateCb& validateCb,size_t size);
};
template <>
struct ValidatorTraits<FieldGetSet::Operation::BufResize>
{
    inline static bool validate(Validator* validator,const Field* field,const ValidateCb& validateCb,size_t size);
};
template <>
struct ValidatorTraits<FieldGetSet::Operation::Unset>
{
    inline static bool validate(Validator* validator,const Field* field,const ValidateCb& validateCb,int);
};
}

/**
 * @brief Base class for unit validators
 */
class Validator
{
    public:

        virtual ~Validator()=default;
        Validator(const Validator&)=default;
        Validator(Validator&&)=default;
        Validator& operator=(const Validator&)=default;
        Validator& operator=(Validator&&)=default;

        virtual bool validateSet(const Field* field,bool val,const ValidateCb& validateCb=ValidateCb()) {std::ignore=val;std::ignore=validateCb;std::ignore=field;return true;}
        virtual bool validateSet(const Field* field,uint8_t val,const ValidateCb& validateCb=ValidateCb()) {std::ignore=val;std::ignore=validateCb;std::ignore=field;return true;}
        virtual bool validateSet(const Field* field,uint16_t val,const ValidateCb& validateCb=ValidateCb()) {std::ignore=val;std::ignore=validateCb;std::ignore=field;return true;}
        virtual bool validateSet(const Field* field,uint32_t val,const ValidateCb& validateCb=ValidateCb()) {std::ignore=val;std::ignore=validateCb;std::ignore=field;return true;}
        virtual bool validateSet(const Field* field,uint64_t val,const ValidateCb& validateCb=ValidateCb()) {std::ignore=val;std::ignore=validateCb;std::ignore=field;return true;}
        virtual bool validateSet(const Field* field,int8_t val,const ValidateCb& validateCb=ValidateCb()) {std::ignore=val;std::ignore=validateCb;std::ignore=field;return true;}
        virtual bool validateSet(const Field* field,int16_t val,const ValidateCb& validateCb=ValidateCb()) {std::ignore=val;std::ignore=validateCb;std::ignore=field;return true;}
        virtual bool validateSet(const Field* field,int32_t val,const ValidateCb& validateCb=ValidateCb()) {std::ignore=val;std::ignore=validateCb;std::ignore=field;return true;}
        virtual bool validateSet(const Field* field,int64_t val,const ValidateCb& validateCb=ValidateCb()) {std::ignore=val;std::ignore=validateCb;std::ignore=field;return true;}
        virtual bool validateSet(const Field* field,float val,const ValidateCb& validateCb=ValidateCb()) {std::ignore=val;std::ignore=validateCb;std::ignore=field;return true;}
        virtual bool validateSet(const Field* field,double val,const ValidateCb& validateCb=ValidateCb()) {std::ignore=val;std::ignore=validateCb;std::ignore=field;return true;}
        virtual bool validateSet(const Field* field,const char* val,const ValidateCb& validateCb=ValidateCb()) {std::ignore=val;std::ignore=validateCb;std::ignore=field;return true;}
        virtual bool validateSet(const Field* field,const std::string& val,const ValidateCb& validateCb=ValidateCb()) {std::ignore=val;std::ignore=validateCb;std::ignore=field;return true;}
        virtual bool validateSet(const Field* field,const common::ConstDataBuf& val,const ValidateCb& validateCb=ValidateCb()) {std::ignore=val;std::ignore=validateCb;std::ignore=field;return true;}

        virtual bool validateArrayResize(const Field* field,size_t size,const ValidateCb& validateCb=ValidateCb()) {std::ignore=size;std::ignore=validateCb;std::ignore=field;return true;}
        virtual bool validateBufResize(const Field* field,size_t size,const ValidateCb& validateCb=ValidateCb()) {std::ignore=size;std::ignore=validateCb;std::ignore=field;return true;}
        virtual bool validateUnset(const Field* field,const ValidateCb& validateCb=ValidateCb()) {std::ignore=validateCb;std::ignore=field;return true;}

        template <FieldGetSet::Operation Op,typename T>
        bool validate(const Field* field,const ValidateCb& validateCb,const T& val)
        {
            return details::ValidatorTraits<Op>::validate(this,field,validateCb,val);
        }

        virtual bool validateUnit(const Unit* unit,const ValidateCb& validateCb=ValidateCb()) {std::ignore=unit;std::ignore=validateCb;return true;}
};

//---------------------------------------------------------------

namespace details
{
template <typename T>
bool ValidatorTraits<FieldGetSet::Operation::Set>::validate(Validator* validator,const Field* field,const ValidateCb& validateCb,const T& val)
{
    return validator->validateSet(field,val,validateCb);
}
inline bool ValidatorTraits<FieldGetSet::Operation::ArrayResize>::validate(Validator* validator,const Field* field,const ValidateCb& validateCb,size_t size)
{
    return validator->validateArrayResize(field,size,validateCb);
}
inline bool ValidatorTraits<FieldGetSet::Operation::BufResize>::validate(Validator* validator,const Field* field,const ValidateCb& validateCb,size_t size)
{
    return validator->validateBufResize(field,size,validateCb);
}
inline bool ValidatorTraits<FieldGetSet::Operation::Unset>::validate(Validator* validator,const Field* field,const ValidateCb& validateCb,int)
{
    return validator->validateUnset(field,validateCb);
HATN_DATAUNIT_NAMESPACE_END

//---------------------------------------------------------------
HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITBASEVALIDATOR_H
