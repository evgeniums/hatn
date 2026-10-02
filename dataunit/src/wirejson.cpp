/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file dataunit/wirejson.cpp
  *
  */

#include <map>
#include <vector>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <hatn/thirdparty/base64/base64.h>

#include <hatn/dataunit/datauniterror.h>
#include <hatn/dataunit/stream.h>
#include <hatn/dataunit/valuetypes.h>
#include <hatn/dataunit/wirejson.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

namespace {

//! Same cap io::deserialize() applies to a length prefix.
constexpr uint64_t MaxWireLength=uint64_t(1)<<28;
//! Largest field number the wire format allows.
constexpr uint64_t MaxFieldNumber=(uint64_t(1)<<29)-1;

struct Node
{
    enum class Kind
    {
        Number,
        Text,
        Bytes,
        Message
    };

    Kind kind=Kind::Number;
    uint64_t number=0;
    const char* data=nullptr;
    size_t size=0;
    //! field number -> occurrences, in wire order
    std::map<uint32_t,std::vector<Node>> fields;
};

bool readVarInt(const char* data, size_t size, size_t& offset, uint64_t& value)
{
    bool moreBytesLeft=false;
    auto consumed=StreamBase::unpackVarInt64(data+offset,size-offset,value,moreBytesLeft);
    if (consumed<=0 || moreBytesLeft)
    {
        return false;
    }
    offset+=static_cast<size_t>(consumed);
    return true;
}

bool isUtf8Text(const char* data, size_t size)
{
    size_t i=0;
    while (i<size)
    {
        auto c=static_cast<unsigned char>(data[i]);
        size_t extra=0;
        uint32_t cp=0;
        if (c<0x80)
        {
            // control characters other than tab, newline and carriage return mean binary
            if (c<0x20 && c!='\t' && c!='\n' && c!='\r')
            {
                return false;
            }
            ++i;
            continue;
        }
        else if ((c&0xE0)==0xC0)
        {
            extra=1;
            cp=c&0x1F;
        }
        else if ((c&0xF0)==0xE0)
        {
            extra=2;
            cp=c&0x0F;
        }
        else if ((c&0xF8)==0xF0)
        {
            extra=3;
            cp=c&0x07;
        }
        else
        {
            return false;
        }
        if (i+extra>=size)
        {
            return false;
        }
        for (size_t j=1;j<=extra;j++)
        {
            auto cc=static_cast<unsigned char>(data[i+j]);
            if ((cc&0xC0)!=0x80)
            {
                return false;
            }
            cp=(cp<<6)|(cc&0x3F);
        }
        // overlong encodings, surrogates and out-of-range code points
        if ((extra==1 && cp<0x80) || (extra==2 && cp<0x800) || (extra==3 && cp<0x10000)
            || (cp>=0xD800 && cp<=0xDFFF) || cp>0x10FFFF)
        {
            return false;
        }
        i+=extra+1;
    }
    return true;
}

bool parseMessage(const char* data, size_t size, size_t depth, size_t maxDepth, Node& out);

void classifyLengthDelimited(const char* data, size_t size, size_t depth, size_t maxDepth, Node& node)
{
    node.data=data;
    node.size=size;

    // A nested message first, as a raw protobuf decoder does: ordinary text rarely parses as one
    // (most bytes decode to an invalid wire type or a length that does not fit).
    if (size!=0 && depth<maxDepth)
    {
        Node nested;
        if (parseMessage(data,size,depth+1,maxDepth,nested))
        {
            node.kind=Node::Kind::Message;
            node.fields=std::move(nested.fields);
            return;
        }
    }
    node.kind=isUtf8Text(data,size) ? Node::Kind::Text : Node::Kind::Bytes;
}

bool parseMessage(const char* data, size_t size, size_t depth, size_t maxDepth, Node& out)
{
    out.kind=Node::Kind::Message;
    size_t offset=0;
    while (offset<size)
    {
        uint64_t tag=0;
        if (!readVarInt(data,size,offset,tag))
        {
            return false;
        }
        auto fieldNumber=tag>>3;
        auto wireType=static_cast<int>(tag&7);
        if (fieldNumber==0 || fieldNumber>MaxFieldNumber)
        {
            return false;
        }

        Node node;
        switch (static_cast<WireType>(wireType))
        {
            case WireType::VarInt:
            {
                if (!readVarInt(data,size,offset,node.number))
                {
                    return false;
                }
                break;
            }
            case WireType::Fixed32:
            {
                if (size-offset<4)
                {
                    return false;
                }
                uint32_t value=0;
                for (size_t i=0;i<4;i++)
                {
                    value|=static_cast<uint32_t>(static_cast<unsigned char>(data[offset+i]))<<(8*i);
                }
                node.number=value;
                offset+=4;
                break;
            }
            case WireType::Fixed64:
            {
                if (size-offset<8)
                {
                    return false;
                }
                uint64_t value=0;
                for (size_t i=0;i<8;i++)
                {
                    value|=static_cast<uint64_t>(static_cast<unsigned char>(data[offset+i]))<<(8*i);
                }
                node.number=value;
                offset+=8;
                break;
            }
            case WireType::WithLength:
            {
                uint64_t length=0;
                if (!readVarInt(data,size,offset,length) || length>MaxWireLength || length>size-offset)
                {
                    return false;
                }
                classifyLengthDelimited(data+offset,static_cast<size_t>(length),depth,maxDepth,node);
                offset+=static_cast<size_t>(length);
                break;
            }
            default:
                // groups (3, 4) and reserved types: not produced by this serializer
                return false;
        }
        out.fields[static_cast<uint32_t>(fieldNumber)].push_back(std::move(node));
    }
    return true;
}

template <typename WriterT>
void writeNode(WriterT& writer, const Node& node)
{
    switch (node.kind)
    {
        case Node::Kind::Number:
            writer.Uint64(node.number);
            break;

        case Node::Kind::Text:
            writer.String(node.data,static_cast<rapidjson::SizeType>(node.size));
            break;

        case Node::Kind::Bytes:
        {
            auto encoded=common::Base64::to<std::string>(node.data,node.size);
            writer.String(encoded.data(),static_cast<rapidjson::SizeType>(encoded.size()));
            break;
        }

        case Node::Kind::Message:
        {
            writer.StartObject();
            for (const auto& field : node.fields)
            {
                auto key=std::to_string(field.first);
                writer.Key(key.data(),static_cast<rapidjson::SizeType>(key.size()));
                if (field.second.size()==1)
                {
                    writeNode(writer,field.second.front());
                }
                else
                {
                    writer.StartArray();
                    for (const auto& item : field.second)
                    {
                        writeNode(writer,item);
                    }
                    writer.EndArray();
                }
            }
            writer.EndObject();
            break;
        }
    }
}

} // anonymous namespace

//---------------------------------------------------------------

common::Result<std::string> wireToJson(const char* data, size_t size, size_t maxDepth)
{
    Node root;
    if (size!=0 && (data==nullptr || !parseMessage(data,size,0,maxDepth,root)))
    {
        return unitError(UnitError::PARSE_ERROR);
    }

    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    root.kind=Node::Kind::Message;
    writeNode(writer,root);
    return std::string{buf.GetString(),buf.GetSize()};
}

//---------------------------------------------------------------

HATN_DATAUNIT_NAMESPACE_END
