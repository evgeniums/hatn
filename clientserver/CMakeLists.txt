PROJECT(hatnclientserver)

SET (HEADERS
    include/hatn/clientserver/clientserver.h
    include/hatn/clientserver/clientservererror.h
    include/hatn/clientserver/clientservererrorcodes.h
    include/hatn/clientserver/usercontroller.h
    include/hatn/clientserver/accountconfigparser.h
)

SET (HEADERS
    ${HEADERS}

    include/hatn/clientserver/models/oid.h
    include/hatn/clientserver/models/user.h
    include/hatn/clientserver/models/withuser.h
    include/hatn/clientserver/models/loginprofile.h
    include/hatn/clientserver/models/usercharacter.h
    include/hatn/clientserver/models/withusercharacter.h
    include/hatn/clientserver/models/withloginprofile.h

    include/hatn/clientserver/models/accountconfig.h
)

SET (SOURCES
    src/clientserver.cpp
    src/accountconfigparser.cpp
    src/clientservererror.cpp
)

BUILD_HATN_MODULE()
