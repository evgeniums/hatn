#pragma once

#include <hatn/clientserver/clientserver.h>

#ifdef __APPLE__
#if TARGET_OS_IPHONE
#define HATN_CLIENT_FILES_DEPRECATED
#elif TARGET_OS_MAC

// #define HATN_CLIENT_FILES_DEPRECATED

#endif
#endif

#ifdef ANDROID
#define HATN_CLIENT_FILES_DEPRECATED
#endif

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

#ifdef HATN_CLIENT_FILES_DEPRECATED

namespace topic_object{}
namespace avatar_object=topic_object;
namespace image_object=topic_object;

#else

namespace uid{}
namespace avatar_object=uid;
namespace image_object=uid;

#endif

HATN_CLIENT_SERVER_NAMESPACE_END
