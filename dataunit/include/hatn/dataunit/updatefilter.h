/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** \file dataunit/unitwrapper.h
  *
  *  Filter of unit updating operations that connects data unit with validator and notifier
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITFILTER_H
#define HATNDATAUNITFILTER_H

#include <functional>

#include <hatn/common/translate.h>
#include <hatn/common/sharedptr.h>

#include <hatn/validator/error.hpp>
#include <hatn/validator/adapters/single_member_adapter.hpp>

#include <hatn/dataunit/unit.h>
#include <hatn/dataunit/updatenotifier.h>

HATN_NAMESPACE_BEGIN
using HATN_COMMON_NS::_TR;
namespace HATN_DATAUNIT_NS {

namespace vld=HATN_VALIDATOR_NAMESPACE;

/**
 * @brief Filter of unit updating operations that connects data unit with validator and notifier
 */
template <typename SharedValidatorT>
class UpdateFilter
{
    public:

        UpdateFilter(
                SharedValidatorT validator,
                std::shared_ptr<UpdateNotifier> notifier
            ) noexcept : m_validator(std::move(validator)),
                         m_notifier(std::move(notifier))
        {}

        explicit UpdateFilter(
                SharedValidatorT validator
            ) noexcept : UpdateFilter(std::move(validator),std::shared_ptr<UpdateNotifier>())
        {}

        explicit UpdateFilter(
                std::shared_ptr<UpdateNotifier> notifier
            ) noexcept : UpdateFilter(SharedValidatorT(),std::move(notifier))
        {}

        UpdateFilter()=default;

        void setValidator(SharedValidatorT validator)
        {
            m_validator=std::move(validator);
        }
        SharedValidatorT validator() const noexcept
        {
            return m_validator;
        }

        void setNotifier(std::shared_ptr<UpdateNotifier> notifier)
        {
            m_notifier=std::move(notifier);
        }
        std::shared_ptr<UpdateNotifier> notifier() const noexcept
        {
            return m_notifier;
        }

        template <typename UnitT, typename ValT, typename... FieldPath>
        vld::error_report set(UnitT* unit, std::initializer_list<FieldPath...> fieldPath, const ValT& val, UpdateInvoker* invoker=nullptr) const
        {
            return vld::error_report();
        }

#if 0
        template <typename T>
        bool set(UpdateInvoker* invoker,Unit* unit,int id, const T& val, const ValidateCb& validateCb=ValidateCb()) const
        {
            auto handler=[](Field* field, const T& val)
            {
                field->setValue(val);
            };
            return update<FieldGetSet::Operation::Set>(invoker,unit,id,handler,validateCb,val);
        }
        bool set(UpdateInvoker* invoker,Unit* unit,int id,const char* data, size_t length, const ValidateCb& validateCb=ValidateCb()) const
        {
            return set(invoker,unit,id,common::ConstDataBuf(data,length),validateCb);
        }
        bool unset(UpdateInvoker* invoker,Unit* unit,int id, const ValidateCb& validateCb=ValidateCb()) const
        {
            auto handler=[](Field* field,int)
            {
                field->clear();
            };
            return update<FieldGetSet::Operation::Unset>(invoker,unit,id,handler,validateCb,0);
        }

        bool bufResize(UpdateInvoker* invoker,Unit* unit,int id, size_t size, const ValidateCb& validateCb=ValidateCb()) const
        {
            auto handler=[](Field* field, size_t size)
            {
                field->bufResize(size);
            };
            return update<FieldGetSet::Operation::BufResize>(invoker,unit,id,handler,validateCb,size);
        }
        bool bufClear(UpdateInvoker* invoker,Unit* unit,int id, const ValidateCb& validateCb=ValidateCb()) const
        {
            auto handler=[](Field* field, int)
            {
                field->bufClear();
            };
            return update<FieldGetSet::Operation::BufResize>(invoker,unit,id,handler,validateCb,0);
        }

        template <typename T>
        bool arraySet(UpdateInvoker* invoker,Unit* unit,int id, size_t idx, const T& val, const ValidateCb& validateCb=ValidateCb()) const
        {
            auto handler=[idx](Field* field, const T& val)
            {
                field->arraySet(idx,val);
            };
            return update<FieldGetSet::Operation::Set>(invoker,unit,id,handler,validateCb,val);
        }
        bool arrayResize(UpdateInvoker* invoker,Unit* unit,int id, size_t size, const ValidateCb& validateCb=ValidateCb(), bool doResize=true) const
        {
            auto handler=[doResize](Field* field, size_t size)
            {
                if (doResize)
                {
                    field->arrayResize(size);
                }
            };
            return update<FieldGetSet::Operation::ArrayResize>(invoker,unit,id,handler,validateCb,size);
        }
        bool arrayClear(UpdateInvoker* invoker,Unit* unit,int id, const ValidateCb& validateCb=ValidateCb()) const
        {
            auto handler=[](Field* field,int)
            {
                field->arrayClear();
            };
            return update<FieldGetSet::Operation::ArrayResize>(invoker,unit,id,handler,validateCb,0);
        }
        template <typename T>
        bool arrayAdd(UpdateInvoker* invoker,Unit* unit,int id, const T& val, const ValidateCb& validateCb=ValidateCb()) const
        {
            auto field=unit->fieldById(id);
            if (field!=nullptr)
            {
                if (!arrayResize(invoker,unit,id,field->arraySize()+1,validateCb,false))
                {
                    return false;
                }
            }
            auto handler=[](Field* field, const T& val)
            {
                field->arrayAdd(val);
            };
            return update<FieldGetSet::Operation::Set>(invoker,unit,id,handler,validateCb,val);
        }

        bool arrayBufResize(UpdateInvoker* invoker,Unit* unit,int id, size_t idx, size_t size, const ValidateCb& validateCb=ValidateCb()) const
        {
            auto handler=[idx](Field* field, size_t size)
            {
                field->arrayBufResize(idx,size);
            };
            return update<FieldGetSet::Operation::BufResize>(invoker,unit,id,handler,validateCb,size);
        }
        bool arrayBufClear(UpdateInvoker* invoker,Unit* unit,int id, size_t idx, const ValidateCb& validateCb=ValidateCb()) const
        {
            auto handler=[idx](Field* field,int)
            {
                field->arrayBufClear(idx);
            };
            return update<FieldGetSet::Operation::BufResize>(invoker,unit,id,handler,validateCb,0);
        }
        bool arrayBufSet(UpdateInvoker* invoker,Unit* unit,int id, size_t idx, const char* data, size_t length,const ValidateCb& validateCb=ValidateCb()) const
        {
            return arrayBufSet(invoker,unit,id,idx,common::ConstDataBuf(data,length),validateCb);
        }
        bool arrayBufSet(UpdateInvoker* invoker,Unit* unit,int id, size_t idx, const common::ConstDataBuf& buf,const ValidateCb& validateCb=ValidateCb()) const
        {
            auto handler=[idx](Field* field, const common::ConstDataBuf& buf)
            {
                field->arrayBufSetValue(idx,buf);
            };
            return update<FieldGetSet::Operation::Set>(invoker,unit,id,handler,validateCb,buf);
        }
        bool arrayBufAdd(UpdateInvoker* invoker,Unit* unit,int id, const char* data, size_t length,const ValidateCb& validateCb=ValidateCb()) const
        {
            return arrayBufAdd(invoker,unit,id,common::ConstDataBuf(data,length),validateCb);
        }
        bool arrayBufAdd(UpdateInvoker* invoker,Unit* unit,int id, const common::ConstDataBuf& buf,const ValidateCb& validateCb=ValidateCb()) const
        {
            auto field=unit->fieldById(id);
            if (field!=nullptr)
            {
                if (!arrayResize(invoker,unit,id,field->arraySize()+1,validateCb,false))
                {
                    return false;
                }
            }
            auto handler=[](Field* field, const common::ConstDataBuf& buf)
            {
                field->arrayBufAddValue(buf);
            };
            return update<FieldGetSet::Operation::Set>(invoker,unit,id,handler,validateCb,buf);
        }

        void notifyUpdate(UpdateInvoker* invoker,Field* field) const
        {
            if (m_notifier)
            {
                m_notifier->notify(invoker,field);
            }
        }
        void beginGroupUpdate(UpdateInvoker* invoker,Unit* unit,size_t groupID) const
        {
            if (m_notifier)
            {
                m_notifier->beginGroupUpdate(invoker,unit,groupID);
            }
        }
        void endGroupUpdate(UpdateInvoker* invoker,Unit* unit,size_t groupID) const
        {
            if (m_notifier)
            {
                m_notifier->endGroupUpdate(invoker,unit,groupID);
            }
        }

    private:

        template <FieldGetSet::Operation Op,typename HandlerT,typename T>
        bool update(UpdateInvoker* invoker,Unit* unit,int id, const HandlerT& handler,const ValidateCb& validateCb, const T& val) const
        {
            auto field=unit->fieldById(id);
            if (field!=nullptr)
            {
                if (m_validator && !m_validator->validate<Op>(field,validateCb,val))
                {
                    return false;
                }

                handler(field,val);
                notifyUpdate(invoker,field);
                return true;
            }
            if (validateCb)
            {
                validateCb(false,fmt::format(_TR("Field with ID={} is not present in object {}","dataunit"),id,unit->name()));
            }
            return false;
        }
#endif

    private:

        SharedValidatorT m_validator;
        std::shared_ptr<UpdateNotifier> m_notifier;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITFILTER_H
