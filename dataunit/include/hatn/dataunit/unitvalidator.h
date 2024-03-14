/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** \file dataunit/unitvalidator.h
  *
  *  Default implementation of unit validators
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITVALIDATOR_H
#define HATNDATAUNITVALIDATOR_H

#include <type_traits>
#include <functional>

#include <hatn/common/translate.h>
#include <hatn/dataunit/field.h>
#include <hatn/dataunit/unit.h>
#include <hatn/dataunit/validator.h>
#include <hatn/dataunit/fieldvalidators.h>
#include <hatn/dataunit/stringvalidators.h>

HATN_NAMESPACE_BEGIN
using HATN_COMMON_NS::_TR;
namespace HATN_DATAUNIT_NS {

/**
 * @brief Default implementation of unit validators
 */
template <typename ... Validators>
class UnitValidator : public Validator
{
    public:

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

        virtual bool validateUnit(const Unit* unit,const ValidateCb& validateCb=ValidateCb()) {std::ignore=unit;std::ignore=validateCb;return true;}

    private:

        std::tuple<Validators> m_fieldValidators;
};

//---------------------------------------------------------------
HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITVALIDATOR_H
