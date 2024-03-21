/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/configtreejson.сpp
  *
  *  Contains implementation of JSON parser and serializer for configuration tree.
  *
*/

#include <cstddef>
#ifndef RAPIDJSON_NO_SIZETYPEDEFINE
#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson { using SizeType=size_t; }
#endif

#include <stack>
#include <memory>
#include <string>

#include <rapidjson/reader.h>
#include "rapidjson/error/en.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include <hatn/common/error.h>
#include <hatn/common/utils.h>

#include <hatn/dataunit/rapidjsonstream.h>

#include <hatn/base/configtree.h>
#include <hatn/base/configtreejson.h>

HATN_BASE_NAMESPACE_BEGIN

/****************************Parser******************************************/

namespace {

struct Context
{
    std::shared_ptr<ConfigTree> tree;
    config_tree::MapT* map=nullptr;

    bool inArray=false;
};

struct Parser
{
    std::stack<std::shared_ptr<Context>> stack;
    std::string error;
};

struct ReaderHandler : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, ReaderHandler>
{
    ReaderHandler(Parser& parser):parser(parser)
    {}

    Parser& parser;

    Result<std::shared_ptr<Context>&> context()
    {
        if (parser.stack.empty())
        {
            auto&& err=makeSystemError(std::errc::result_out_of_range);
            parser.error=err.message();
            return ErrorResult(std::move(err));
        }
        return parser.stack.top();
    }

    bool StartObject()
    {
        auto ctx=context();
        HATN_BOOL_RESULT(ctx)
        auto& current=ctx.value();

        if (!current->inArray)
        {
            auto r=current->tree->toMap();
            HATN_BOOL_RESULT_MSG(r,parser.error)
            current->map=&r.value();
            return true;
        }

        if (!current->tree->isSet())
        {
            auto r=current->tree->toArray<ConfigTree>();
            HATN_BOOL_RESULT_MSG(r,parser.error)
        }

        auto arr=current->tree->asArray<ConfigTree>();
        HATN_BOOL_RESULT_MSG(arr,parser.error)

        auto next=std::make_shared<Context>();
        next->tree=std::make_shared<ConfigTree>();
        arr->append(next->tree);

        auto r1=next->tree->toMap();
        HATN_BOOL_RESULT_MSG(r1,parser.error)
        next->map=&r1.value();

        parser.stack.push(std::move(next));

        return true;
    }

    bool EndObject(rapidjson::SizeType)
    {
        if (parser.stack.empty())
        {
            parser.error="stack empty";
            return false;
        }
        parser.stack.pop();
        return true;
    }

    bool Key(const Ch* str, rapidjson::SizeType length, bool)
    {
        auto ctx=context();
        HATN_BOOL_RESULT(ctx)
        auto& current=ctx.value();

        if (current->map==nullptr)
        {
            parser.error="key not within object";
            return false;
        }
        auto next=std::make_shared<Context>();
        next->tree=std::make_shared<ConfigTree>();
        current->map->emplace(std::string(static_cast<const char*>(str),length),next->tree);
        parser.stack.push(std::move(next));

        return true;
    }

    bool Null()
    {
        if (parser.stack.empty())
        {
            parser.error="stack empty";
            return false;
        }
        parser.stack.pop();
        return true;
    }

    template <typename T>
    bool setValue(T&& v)
    {
        auto ctx=context();
        HATN_BOOL_RESULT(ctx)
        auto& current=ctx.value();

        if (!current->inArray)
        {
            current->tree->set(std::forward<T>(v));
            parser.stack.pop();
        }
        else
        {
            if (!current->tree->isSet())
            {
                auto r=current->tree->toArray<std::decay_t<T>>();
                HATN_BOOL_RESULT_MSG(r,parser.error)
            }

            auto arrayTypeId=config_tree::ValueType<std::decay_t<T>>::arrayId;
            if (current->tree->type()!=arrayTypeId)
            {
                parser.error=fmt::format("mismatched types of array elements (note, floating numbers must contain period (.) symbol)");
                return false;
            }

            auto arr=current->tree->asArray<std::decay_t<T>>();
            HATN_BOOL_RESULT_MSG(arr,parser.error)
            arr->append(std::forward<T>(v));
        }

        return true;
    }

    bool Bool(bool b)
    {
        return setValue(b);
    }

    bool Int(int i)
    {
        return setValue(i);
    }

    bool Uint(unsigned i)
    {
        return setValue(i);
    }

    bool Int64(int64_t i)
    {
        return setValue(i);
    }

    bool Uint64(uint64_t i)
    {
        return setValue(i);
    }

    bool Double(double d)
    {
        return setValue(d);
    }

    bool String(const Ch* str, rapidjson::SizeType length, bool)
    {
        return setValue(std::string(static_cast<const char*>(str),length));
    }

    bool StartArray()
    {
        auto ctx=context();
        HATN_BOOL_RESULT(ctx)
        auto& current=ctx.value();

        // check if it is array of arrays
        if (!current->inArray)
        {
            // current tree must be not initialized
            if (current->tree->isSet())
            {
                parser.error="invalid state for array of arrays";
                return false;
            }

            // open array
            current->inArray=true;
            return true;
        }

        // open new child array in current array (array of arrays)

        // make new array if not set
        if (!current->tree->isSet())
        {
            auto r=current->tree->toArray<ConfigTree>();
            HATN_BOOL_RESULT_MSG(r,parser.error)
        }

        // add subarray
        auto arr=current->tree->asArray<ConfigTree>();
        HATN_BOOL_RESULT_MSG(arr,parser.error)
        auto next=std::make_shared<Context>();
        next->inArray=true;
        next->tree=std::make_shared<ConfigTree>();
        arr->append(next->tree);
        parser.stack.push(std::move(next));

        return true;
    }

    bool EndArray(rapidjson::SizeType)
    {
        auto ctx=context();
        HATN_BOOL_RESULT(ctx)
        auto& current=ctx.value();

        if (!current->inArray)
        {
            parser.error="end of array not within array";
            return false;
        }

        parser.stack.pop();
        return true;
    }
};

} // anonymous namespace

//---------------------------------------------------------------

Error ConfigTreeJson::doParse(
        ConfigTree& target,
        const common::lib::string_view& source,
        const ConfigTreePath& root,
        const std::string&
    ) const noexcept
{
    //! @todo check format

    Parser parser;
    auto rootCtx=std::make_shared<Context>();
    rootCtx->tree=std::make_shared<ConfigTree>();
    parser.stack.push(rootCtx);

    ReaderHandler handler{parser};
    dataunit::RapidJsonStringViewStream ss(source);

    rapidjson::Reader reader;
    if (reader.Parse<rapidjson::kParseStopWhenDoneFlag |
                     rapidjson::kParseCommentsFlag |
                     rapidjson::kParseTrailingCommasFlag>
            (ss, handler)
       )
    {
        if (root.isRoot() && !target.isSet())
        {
            target=std::move(*rootCtx->tree);
        }
        else
        {
            return CommonError::NOT_IMPLEMENTED;
        }
        // else
        // {
        //     return target.merge(*rootCtx->tree,root);
        // }
    }
    else {
        auto e = reader.GetParseErrorCode();
        size_t offset = reader.GetErrorOffset();
        if (parser.error.empty())
        {
            parser.error=rapidjson::GetParseError_En(e);
        }

        auto msg=fmt::format("{} at offset {} near {}...", parser.error, offset, source.substr(offset, 32));
        auto ec=Error{BaseError::CONFIG_PARSE_ERROR,std::make_shared<ConfigTreeParseError>(msg,offset,static_cast<int>(e))};
        return ec;
    }

    return OK;
}

//---------------------------------------------------------------

/****************************Serializer**************************************/

namespace {

void writeTree(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, const ConfigTree* current)
{
    if (!current->isSet(true))
    {
        writer.Null();
        return;
    }

    switch (current->type(true))
    {
        case(config_tree::Type::Int):
        {
            switch (current->numericType(true))
            {
                case(config_tree::NumericType::Int8):
                    writer.Int(current->as<int8_t>().value());break;
                case(config_tree::NumericType::Int16):
                    writer.Int(current->as<int16_t>().value());break;
                case(config_tree::NumericType::Int32):
                    writer.Int(current->as<int32_t>().value());break;
                case(config_tree::NumericType::Int64):
                    writer.Int64(current->as<int64_t>().value());break;
                case(config_tree::NumericType::UInt8):
                    writer.Uint(current->as<uint8_t>().value());break;
                case(config_tree::NumericType::UInt16):
                    writer.Uint(current->as<uint16_t>().value());break;
                case(config_tree::NumericType::UInt32):
                    writer.Uint(current->as<uint32_t>().value());break;
                case(config_tree::NumericType::UInt64):
                    writer.Uint64(current->as<uint64_t>().value());break;
                case(config_tree::NumericType::None):break;
            }
            break;
        }

        case(config_tree::Type::String):
        {
            writer.String(current->as<std::string>()->c_str());
            break;
        }

        case(config_tree::Type::Bool):
        {
            writer.Bool(current->as<bool>().value());
            break;
        }

        case(config_tree::Type::Double):
        {
            writer.Double(current->as<double>().value());
            break;
        }

        case(config_tree::Type::Map):
        {
            writer.StartObject();
            auto m=current->asMap();
            for (auto&& it: m.value())
            {
                writer.Key(it.first.c_str());
                writeTree(writer,it.second.get());
            }
            writer.EndObject();
            break;
        }

        case(config_tree::Type::ArrayInt):
        {
            writer.StartArray();

            switch (current->numericType(true))
            {
                case(config_tree::NumericType::Int8):
                {
                    auto arr=current->asArray<int8_t>();
                    for (size_t i=0;i<arr->size();i++)
                    {
                        writer.Int(arr->at(i));
                    }
                    break;
                }
                case(config_tree::NumericType::Int16):
                {
                    auto arr=current->asArray<int16_t>();
                    for (size_t i=0;i<arr->size();i++)
                    {
                        writer.Int(arr->at(i));
                    }
                    break;
                }
                case(config_tree::NumericType::Int32):
                {
                    auto arr=current->asArray<int32_t>();
                    for (size_t i=0;i<arr->size();i++)
                    {
                        writer.Int(arr->at(i));
                    }
                    break;
                }
                case(config_tree::NumericType::Int64):
                {
                    auto arr=current->asArray<int64_t>();
                    for (size_t i=0;i<arr->size();i++)
                    {
                        writer.Int64(arr->at(i));
                    }
                    break;
                }
                case(config_tree::NumericType::UInt8):
                {
                    auto arr=current->asArray<uint8_t>();
                    for (size_t i=0;i<arr->size();i++)
                    {
                        writer.Uint(arr->at(i));
                    }
                    break;
                }
                case(config_tree::NumericType::UInt16):
                {
                    auto arr=current->asArray<uint16_t>();
                    for (size_t i=0;i<arr->size();i++)
                    {
                        writer.Uint(arr->at(i));
                    }
                    break;
                }
                case(config_tree::NumericType::UInt32):
                {
                    auto arr=current->asArray<uint32_t>();
                    for (size_t i=0;i<arr->size();i++)
                    {
                        writer.Uint(arr->at(i));
                    }
                    break;
                }
                case(config_tree::NumericType::UInt64):
                {
                    auto arr=current->asArray<uint64_t>();
                    for (size_t i=0;i<arr->size();i++)
                    {
                        writer.Uint64(arr->at(i));
                    }
                    break;
                }
                case(config_tree::NumericType::None):break;
            }

            writer.EndArray();
            break;
        }

        case(config_tree::Type::ArrayString):
        {
            writer.StartArray();
            auto arr=current->asArray<std::string>();
            for (size_t i=0;i<arr->size();i++)
            {
                writer.String(arr->at(i).c_str());
            }
            writer.EndArray();
            break;
        }

        case(config_tree::Type::ArrayBool):
        {
            writer.StartArray();
            auto arr=current->asArray<bool>();
            for (size_t i=0;i<arr->size();i++)
            {
                writer.Bool(arr->at(i));
            }
            writer.EndArray();
            break;
        }

        case(config_tree::Type::ArrayDouble):
        {
            writer.StartArray();
            auto arr=current->asArray<double>();
            for (size_t i=0;i<arr->size();i++)
            {
                writer.Double(arr->at(i));
            }
            writer.EndArray();
            break;
        }

        case(config_tree::Type::ArrayTree):
        {
            writer.StartArray();
            auto arr=current->asArray<ConfigTree>();
            for (size_t i=0;i<arr->size();i++)
            {
                const auto& element=arr->at(i);
                writeTree(writer,element.get());
            }
            writer.EndArray();
            break;
        }

        case(config_tree::Type::None):
        {
            writer.Null();
            break;
        }
    }
}

}

//---------------------------------------------------------------

Result<std::string> ConfigTreeJson::doSerialize(
        const ConfigTree& source,
        const ConfigTreePath& root,
        const std::string&
    ) const noexcept
{
    rapidjson::StringBuffer s;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer{s};

    const auto* configTree=&source;
    if (!root.isRoot())
    {
        if (!configTree->isSet(root))
        {
            ConfigTree emptyTree;
            writeTree(writer,&emptyTree);
            return s.GetString();
        }
        else
        {
            auto r=configTree->get(root);
            HATN_CHECK_RESULT(r)
            configTree=&r.value();
        }
    }

    writeTree(writer,configTree);

    return s.GetString();
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END
