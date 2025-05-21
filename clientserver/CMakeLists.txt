PROJECT(hatnclientserver)

SET (HEADERS
    include/hatn/clientserver/clientserver.h
    include/hatn/clientserver/usercontroller.h

    include/hatn/clientserver/models/oid.h
    include/hatn/clientserver/models/user.h
    include/hatn/clientserver/models/withuser.h
    include/hatn/clientserver/models/loginprofile.h
    include/hatn/clientserver/models/usercharacter.h
    include/hatn/clientserver/models/withusercharacter.h
    include/hatn/clientserver/models/withloginprofile.h
)

SET (SOURCES
    src/clientserver.cpp
)

BUILD_HATN_MODULE()
