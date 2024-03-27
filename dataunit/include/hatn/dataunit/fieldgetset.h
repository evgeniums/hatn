/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/fieldgetset.h
  *
  *  Getter and setter methods for data unit fields
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITFIELDGETSET_H
#define HATNDATAUNITFIELDGETSET_H

#include <cstdint>
#include <cstddef>

#include <hatn/common/utils.h>
#include <hatn/dataunit/dataunit.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

class Unit;

class FieldGetSet
{
    public:

        enum class Operation : int
        {
            Set,
            Unset,
            ArrayResize,
            BufResize
        };

        FieldGetSet()=default;
        virtual ~FieldGetSet()=default;
        FieldGetSet(const FieldGetSet&)=default;
        FieldGetSet(FieldGetSet&&) =default;
        FieldGetSet& operator=(const FieldGetSet&)=default;
        FieldGetSet& operator=(FieldGetSet&&) =default;

        // scalar fields

        virtual bool less(bool val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool less(uint8_t val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool less(uint16_t val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool less(uint32_t val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool less(uint64_t val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool less(int8_t val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool less(int16_t val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool less(int32_t val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool less(int64_t val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool less(float val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool less(double val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}

        virtual bool equals(bool val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool equals(uint8_t val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool equals(uint16_t val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool equals(uint32_t val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool equals(uint64_t val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool equals(int8_t val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool equals(int16_t val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool equals(int32_t val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool equals(int64_t val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool equals(float val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}
        virtual bool equals(double val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;return false;}

        virtual void setValue(bool val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void setValue(uint8_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void setValue(uint16_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void setValue(uint32_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void setValue(uint64_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void setValue(int8_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void setValue(int16_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void setValue(int32_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void setValue(int64_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void setValue(float val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void setValue(double val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}

        virtual void getValue(bool& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void getValue(uint8_t& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void getValue(uint16_t& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void getValue(uint32_t& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void getValue(uint64_t& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void getValue(int8_t& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void getValue(int16_t& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void getValue(int32_t& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void getValue(int64_t& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void getValue(float& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void getValue(double& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;}

        // byte fields

        virtual bool less(const char* val,size_t length) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=length;return false;}
        virtual bool equals(const char* val,size_t length) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=length;return false;}
        virtual bool lexLess(const char* val,size_t length) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=length;return false;}
        virtual bool lexLessI(const char* val,size_t length) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=length;return false;}
        virtual bool lexEquals(const char* val,size_t length) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=length;return false;}
        virtual bool lexEqualsI(const char* val,size_t length) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=length;return false;}

        virtual void setValue(const char* data,size_t length) {Assert(false,"Invalid operation for field of this type");std::ignore=data;std::ignore=length;}
        inline void setValue(const common::ConstDataBuf& data) {setValue(data.data(),data.size());}

        void bufClear() {bufResize(0);}
        virtual void bufResize(size_t size) {Assert(false,"Invalid operation for field of this type");std::ignore=size;}
        virtual void bufReserve(size_t size) {Assert(false,"Invalid operation for field of this type");std::ignore=size;}
        virtual const char* bufCStr() {Assert(false,"Invalid operation for field of this type");return nullptr;}
        virtual char* bufData() const {Assert(false,"Invalid operation for field of this type");return nullptr;}
        virtual char* bufData() {Assert(false,"Invalid operation for field of this type");return nullptr;}
        virtual size_t bufSize() const {Assert(false,"Invalid operation for field of this type");return 0;}
        virtual size_t bufCapacity() const {Assert(false,"Invalid operation for field of this type");return 0;}
        virtual bool bufEmpty() const {Assert(false,"Invalid operation for field of this type");return true;}
        virtual void bufCreateShared() {Assert(false,"Invalid operation for field of this type");}

        // subunit fields

        virtual const Unit* subunit() const {Assert(false,"Invalid operation for field of this type");return nullptr;}
        virtual Unit* subunit() {Assert(false,"Invalid operation for field of this type");return nullptr;}

        // array fields

        virtual size_t arraySize() const {Assert(false,"Invalid operation for field of this type");return 0;}
        virtual bool arrayEmpty() const {Assert(false,"Invalid operation for field of this type");return true;}
        virtual size_t arrayCapacity() const {Assert(false,"Invalid operation for field of this type");return 0;}
        virtual void arrayResize(size_t size) {Assert(false,"Invalid operation for field of this type");std::ignore=size;}
        virtual void arrayReserve(size_t size) {Assert(false,"Invalid operation for field of this type");std::ignore=size;}
        virtual void arrayClear() {Assert(false,"Invalid operation for field of this type");}

        virtual void arrayAdd(bool val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void arrayAdd(uint8_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void arrayAdd(uint16_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void arrayAdd(uint32_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void arrayAdd(uint64_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void arrayAdd(int8_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void arrayAdd(int16_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void arrayAdd(int32_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void arrayAdd(int64_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void arrayAdd(float val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}
        virtual void arrayAdd(double val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;}

        virtual void arraySet(size_t idx,bool val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arraySet(size_t idx,uint8_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arraySet(size_t idx,uint16_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arraySet(size_t idx,uint32_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arraySet(size_t idx,uint64_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arraySet(size_t idx,int8_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arraySet(size_t idx,int16_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arraySet(size_t idx,int32_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arraySet(size_t idx,int64_t val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arraySet(size_t idx,float val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arraySet(size_t idx,double val) {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}

        virtual void arrayGet(size_t idx,bool& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arrayGet(size_t idx,uint8_t& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arrayGet(size_t idx,uint16_t& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arrayGet(size_t idx,uint32_t& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arrayGet(size_t idx,uint64_t& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arrayGet(size_t idx,int8_t& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arrayGet(size_t idx,int16_t& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arrayGet(size_t idx,int32_t& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arrayGet(size_t idx,int64_t& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arrayGet(size_t idx,float& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}
        virtual void arrayGet(size_t idx,double& val) const {Assert(false,"Invalid operation for field of this type");std::ignore=val;std::ignore=idx;}

        void arrayBufClear(size_t idx) {arrayBufResize(idx,0);}
        virtual void arrayBufResize(size_t idx,size_t size) {Assert(false,"Invalid operation for field of this type");std::ignore=idx;std::ignore=size;}
        virtual void arrayBufReserve(size_t idx,size_t size) {Assert(false,"Invalid operation for field of this type");std::ignore=idx;std::ignore=size;}
        virtual const char* arrayBufCStr(size_t idx) const {Assert(false,"Invalid operation for field of this type");std::ignore=idx;}
        virtual const char* arrayBufData(size_t idx) const {Assert(false,"Invalid operation for field of this type");std::ignore=idx;}
        virtual char* arrayBufData(size_t idx) {Assert(false,"Invalid operation for field of this type");std::ignore=idx;}
        virtual size_t arrayBufSize(size_t idx) const {Assert(false,"Invalid operation for field of this type");std::ignore=idx;return 0;}
        virtual size_t arrayBufCapacity(size_t idx) const {Assert(false,"Invalid operation for field of this type");std::ignore=idx;return 0;}
        virtual bool arrayBufEmpty(size_t idx) const {Assert(false,"Invalid operation for field of this type");std::ignore=idx;return true;}
        virtual void arrayBufSetValue(size_t idx,const char* data,size_t length) {Assert(false,"Invalid operation for field of this type");std::ignore=idx;std::ignore=data;std::ignore=length;}
        virtual void arrayBufAddValue(const char* data,size_t length) {Assert(false,"Invalid operation for field of this type");std::ignore=data;std::ignore=length;}
        inline void arrayBufSetValue(size_t idx,const common::ConstDataBuf& data) {arrayBufSetValue(idx,data.data(),data.size());}
        inline void arrayBufAddValue(const common::ConstDataBuf& data) {arrayBufAddValue(data.data(),data.size());}
        virtual void arrayBufCreateShared(size_t idx) {Assert(false,"Invalid operation for field of this type");std::ignore=idx;}

        virtual Unit* arraySubunit(size_t idx) {Assert(false,"Invalid operation for field of this type");std::ignore=idx; return nullptr;}
        virtual const Unit* arraySubunit(size_t idx) const {Assert(false,"Invalid operation for field of this type");std::ignore=idx; return nullptr;}
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITFIELDGETSET_H
