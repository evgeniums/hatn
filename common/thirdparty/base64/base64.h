/****************************************************************************/
/*
    
*/
/** @file base64.h
 *     Encoding/decoding data into and from a base64-encoded format
 */
/****************************************************************************/

#ifndef BASE64_H
#define BASE64_H

#include <string.h>
#include <string>
#include <vector>
#include <stddef.h>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

struct HATN_COMMON_EXPORT Base64Init
{
    Base64Init();
};

class HATN_COMMON_EXPORT Base64
{
    public:

        static size_t decodeReserverLen(size_t size);
        static size_t decode(const char *src, size_t len, char* out);

        static size_t encodeReserverLen(size_t size);
        static size_t encode(const char *src, size_t len, char* out);

        //---------------------------------------------------------------
        template <typename T> static void to(const char *ptr, size_t size, T& buf, bool append=false)
        {
            if( size > 0 )
            {
                size_t prevSize=0;
                if (append)
                {
                    prevSize=buf.size();
                }
                buf.resize(encodeReserverLen(size)+prevSize);
                auto data=buf.data()+prevSize;
                auto resultSize=encode(ptr,size,data);
                if (resultSize!=0)
                {
                    buf.resize(resultSize+prevSize);
                }
                else
                {
                    buf.resize(prevSize);
                }
            }
        }

        //---------------------------------------------------------------
        template <typename T> inline static T to(const char *ptr, size_t size)
        {
            T result;
            to(ptr,size,result);
            return result;
        }

        template <typename T,typename T1=T> inline static T to(const T1 &s) {
            return to<T>(s.data(), s.size());
        }

        template <typename T> inline static T to(const char* s) {
            return to<T>(s, strlen(s));
        }

        template <typename T> static void from(const char *ptr, size_t size, T& buf)
        {
            auto dstSize=decodeReserverLen(size);
            if (dstSize!=0)
            {
                buf.resize(dstSize);
                dstSize=decode(ptr,size,buf.data());
                if (dstSize!=0)
                {
                    buf.resize(dstSize);
                }
                else
                {
                    buf.clear();
                }
            }
        }

        template <typename T> static T from(const char *ptr, size_t size)
        {
            T result;
            from(ptr,size,result);
            return result;
        }

        template <typename T,typename T1=T> static T1 from(const T &s) {
            return from(s.data(), s.size());
        }

    private:

        static Base64Init init;
};

}}
#endif // BASE64_H
