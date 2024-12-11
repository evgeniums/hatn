/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/x509certificate.cpp
 * 	Wrapper/container of X.509 certificate
 */
/****************************************************************************/

#include <sstream>
#include <iomanip>

#include <hatn/common/utils.h>
#include <hatn/common/logger.h>
#include <hatn/common/containerutils.h>

#include <hatn/crypt/x509certificate.h>

namespace hatn {

using namespace common;

namespace crypt {

/*********************** X509Certificate ***********************/

//---------------------------------------------------------------
std::string X509Certificate::toString(bool pretty, bool withKey) const
{
    try
    {
        size_t level=0;
        std::map<size_t,size_t> counts;

        auto incLevel=[&level,&counts]()
        {
            ++level;
            counts[level]=0;
        };
        auto decLevel=[&level,&counts]()
        {
            counts.erase(level);
            --level;
        };
        auto count=[&level,&counts]()
        {
            return counts[level];
        };
        auto incCount=[&level,&counts]()
        {
            counts[level]++;
        };

        std::string ret;

        auto appendField=[pretty,&ret,&level,&count,&incCount](const std::string& fieldName, const std::string& fieldVal)
        {
            if (fieldName!="}" && fieldName!="]")
            {
                if (count()>0)
                {
                    ret+=",";
                }
            }
            if (pretty)
            {
                ret+="\n";
            }
            incCount();
            if (pretty)
            {
                for (size_t i=0;i<level;i++)
                {
                    ret+="    ";
                }
            }
            if (fieldName=="}" || fieldName=="]" || fieldName=="{" || fieldName=="[")
            {
                ret+=fieldName;
            }
            else
            {
                if (!fieldName.empty())
                {
                    if (pretty)
                    {
                        ret+=fmt::format("\"{}\" : {}",fieldName,fieldVal);
                    }
                    else
                    {
                        ret+=fmt::format("\"{}\":{}",fieldName,fieldVal);
                    }
                }
                else
                {
                    ret+=fieldVal;
                }
            }
        };
        auto valStr=[](const std::string& val)
        {
            return fmt::format("\"{}\"",val);
        };

        auto appendAltNames=[this,&incLevel,&decLevel,&appendField,&valStr](bool issuer,const std::string& name, AltNameType type)
        {
            try
            {
                std::vector<std::string> names;
                if (issuer)
                {
                    names=issuerAltNames(type);
                }
                else
                {
                    names=subjectAltNames(type);
                }
                if (!names.empty())
                {
                    appendField(name,"[");
                    incLevel();
                    for (auto&& it:names)
                    {
                        appendField("",valStr(it));
                    }
                    decLevel();
                    appendField("]","");
                }
            }
            catch (...)
            {}
        };
        auto appendAltNameList=[&incLevel,&decLevel,&appendField,&appendAltNames](bool issuer)
        {
            appendField("alt_names","{");
            incLevel();
            appendAltNames(issuer,"dns",AltNameType::DNS);
            appendAltNames(issuer,"email",AltNameType::Email);
            appendAltNames(issuer,"uri",AltNameType::URI);
            appendAltNames(issuer,"ip",AltNameType::IP);
            decLevel();
            appendField("}","");
        };

        auto appendObjectField=[this,&appendField,&valStr](bool issuer,const std::string& fieldname,BasicNameField field)
        {
            if (issuer)
            {
                appendField(fieldname,valStr(issuerNameField(field)));
            }
            else
            {
                appendField(fieldname,valStr(subjectNameField(field)));
            }
        };

        auto appendObject=[&appendField,&appendObjectField,&incLevel,&decLevel,&appendAltNameList](bool issuer)
        {
            if (issuer)
            {
                appendField("issuer","{");
            }
            else
            {
                appendField("subject","{");
            }
            incLevel();
            appendObjectField(issuer,"common_name",BasicNameField::CommonName);
            appendObjectField(issuer,"country",BasicNameField::Country);
            appendObjectField(issuer,"state_or_province",BasicNameField::StateOrProvince);
            appendObjectField(issuer,"locality",BasicNameField::Locality);
            appendObjectField(issuer,"organization",BasicNameField::Organization);
            appendObjectField(issuer,"organization_unit",BasicNameField::OrganizationUnit);
            appendObjectField(issuer,"email_address",BasicNameField::EmailAddress);
            appendAltNameList(issuer);
            decLevel();
            appendField("}","");
        };

        ret+="{";
        incLevel();

        auto v=version();
        appendField("version",fmt::format("\"{} (0x{:x})\"",v+1,v));
        appendField("serial",valStr(formatSerial()));
        appendField("valid_not_before",valStr(formatTime(validNotBefore())));
        appendField("valid_not_after",valStr(formatTime(validNotAfter())));

        appendObject(false);
        appendObject(true);
        if (withKey)
        {
            auto key=publicKey();
            if (key)
            {
                ByteArray buf;
                auto ec=key->exportToBuf(buf,ContainerFormat::PEM);
                if (!ec)
                {
                    appendField("public_key",fmt::format("\n\"\n{}\"",buf.c_str()));
                }
            }
        }

        decLevel();
        ret+="\n}";
        return ret;
    }
    catch (...)
    {}
    return std::string();
}

//---------------------------------------------------------------
std::string X509Certificate::formatSerial(Error *error) const
{
    std::string ret;
    common::FixedByteArray20 buf=serial(error);
    if (error && *error)
    {
        return ret;
    }

    std::string bufHex;
    ContainerUtils::rawToHex(buf.stringView(),bufHex);
    for (size_t i=0;i<bufHex.size();i++)
    {
        if (i!=0 && i%2==0)
        {
            ret+=":";
        }
        ret+=bufHex[i];
    }

    return ret;
}

//---------------------------------------------------------------
std::string X509Certificate::formatTime(const TimePoint &tp)
{
    auto tm = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&tm),"%Y-%m-%d %H:%M:%S GMT");
    return ss.str();
}

/*********************** X509VerifyError ***********************/

//---------------------------------------------------------------
bool X509VerifyError::compareContent(const NativeError &other) const noexcept
{
    auto otherErr=dynamic_cast<const X509VerifyError*>(&other);
    if (!otherErr)
    {
        return false;
    }
    if (otherErr->m_certificate.isNull() && this->m_certificate.isNull())
    {
        return true;
    }
    if (
        (!otherErr->m_certificate.isNull() && this->m_certificate.isNull())
            ||
        (otherErr->m_certificate.isNull() && !this->m_certificate.isNull())
      )
    {
        return false;
    }
    return *otherErr->m_certificate==*m_certificate;
}

//---------------------------------------------------------------
Error X509VerifyError::serializeAppend(ByteArray &buf) const
{
    if (!m_certificate.isNull())
    {
        ByteArray tmpBuf;
        HATN_CHECK_RETURN(m_certificate->exportToBuf(tmpBuf,ContainerFormat::DER))
        buf.append(tmpBuf);
    }
    return Error();
}

//---------------------------------------------------------------
HATN_CRYPT_NAMESPACE_END
