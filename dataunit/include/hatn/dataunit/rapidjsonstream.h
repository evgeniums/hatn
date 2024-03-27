/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/rapidjsonstringviewstream.h
  *
  *      Stream wrapper around string view for rapidjson
  *
  */

/****************************************************************************/

#ifndef HATNRAPIDJSONSTRINGVIEWSTREAM_H
#define HATNRAPIDJSONSTRINGVIEWSTREAM_H

#ifndef RAPIDJSON_NO_SIZETYPEDEFINE
#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson { using SizeType=size_t; }
#endif

#include <rapidjson/reader.h>

#include <hatn/dataunit/dataunit.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

/**
 * @brief Stream wrapper around string view for rapidjson
 */
class HATN_DATAUNIT_EXPORT RapidJsonStringViewStream
{
    public:

    using Ch=char;

    //! Ctor
    RapidJsonStringViewStream(const common::lib::string_view& stringView):m_stringView(stringView),m_cursor(0)
    {}

    //! Read the current character from stream without moving the read cursor.
    Ch Peek() const
    {
        if (m_cursor<m_stringView.size())
        {
            auto ch=m_stringView.at(m_cursor);
            return ch;
        }
        return '\0';
    }

    //! Read the current character from stream and moving the read cursor to next character.
    Ch Take()
    {
        if (m_cursor<m_stringView.size())
        {
            return m_stringView.at(m_cursor++);
        }
        return '\0';
    }

    //! Get the current read cursor.
    //! @return Number of characters read from start.
    size_t Tell()
    {
        return m_cursor;
    }

    //! Begin writing operation at the current read pointer.
    //! @return The begin writer pointer.
    Ch* PutBegin()
    {
        m_cursor=0;
        return nullptr;
    }

    //! Write a character.
    void Put(Ch)
    {
        Assert(false,"Invalid call to Put() to StringViewToStream because the string view is read only");
    }

    //! Flush the buffer.
    void Flush()
    {}

    //! End the writing operation.
    //! @param begin The begin write pointer returned by PutBegin().
    //! @return Number of characters written.
    size_t PutEnd(Ch*)
    {
        return 0;
    }

    private:

        const common::lib::string_view& m_stringView;
        size_t m_cursor;
};

/**
 * @brief Stream wrapper for rapidjson around buffer type
 */
template <typename BufType,size_t RESERVE_SIZE=256>
class HATN_DATAUNIT_EXPORT RapidJsonBufStream
{
    public:

    using Ch=char;

    //! Ctor
    RapidJsonBufStream(BufType& buf):m_buffer(buf),m_cursor(buf.size())
    {}

    //! Read the current character from stream without moving the read cursor.
    Ch Peek() const
    {
        Ch ret='\0';
        if (m_cursor<m_buffer.size())
        {
            m_buffer.at(m_cursor);
        }
        return ret;
    }

    //! Read the current character from stream and moving the read cursor to next character.
    Ch Take()
    {
        Ch ret='\0';
        if (m_cursor<m_buffer.size())
        {
            m_buffer.at(m_cursor++);
        }
        return ret;
    }

    //! Get the current read cursor.
    //! @return Number of characters read from start.
    size_t Tell()
    {
        return m_cursor;
    }

    //! Begin writing operation at the current read pointer.
    //! @return The begin writer pointer.
    Ch* PutBegin()
    {
        size_t freeCapacity=m_buffer.capacity()-m_cursor;
        if (freeCapacity<RESERVE_SIZE)
        {
            m_buffer.reserve(RESERVE_SIZE);
        }
        return m_buffer.data()+m_cursor;
    }

    //! Write a character.
    void Put(Ch character)
    {
        m_buffer.push_back(character);
        m_cursor++;
    }

    //! Flush the buffer.
    void Flush()
    {}

    //! End the writing operation.
    //! @param begin The begin write pointer returned by PutBegin().
    //! @return Number of characters written.
    size_t PutEnd(Ch*)
    {
        return m_cursor;
    }

    Ch* reserve(size_t size)
    {
        m_buffer.resize(m_buffer.size()+size);
        Ch* ptr=const_cast<Ch*>(m_buffer.data())+m_cursor;
        m_cursor+=size;
        return ptr;
    }

    Ch* rollback(size_t size)
    {
        if (size<m_buffer.size())
        {
            m_buffer.resize(m_buffer.size()-size);
            m_cursor-=size;
            return const_cast<Ch*>(m_buffer.data())+m_cursor;
        }
        m_buffer.clear();
        m_cursor=0;
        return nullptr;
    }

    private:

        BufType& m_buffer;
        size_t m_cursor;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNRAPIDJSONSTREAM_H
