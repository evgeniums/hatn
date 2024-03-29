PROJECT(hatndataunit)

SET (HEADERS
    include/hatn/dataunit/dataunit.h
    include/hatn/dataunit/syntax.h
    include/hatn/dataunit/macros.h
    include/hatn/dataunit/types.h
    include/hatn/dataunit/allocatorfactory.h
    include/hatn/dataunit/wiredata.h
    include/hatn/dataunit/wiredatapack.h
    include/hatn/dataunit/stream.h
    include/hatn/dataunit/unit.h
    include/hatn/dataunit/fieldgetset.h
    include/hatn/dataunit/field.h
    include/hatn/dataunit/rapidjsonstream.h
    include/hatn/dataunit/rapidjsonsaxhandlers.h

    include/hatn/dataunit/fields/fieldtraits.h
    include/hatn/dataunit/fields/scalar.h
    include/hatn/dataunit/fields/bytes.h
    include/hatn/dataunit/fields/subunit.h
    include/hatn/dataunit/fields/repeated.h

    include/hatn/dataunit/unittraits.h

    include/hatn/dataunit/detail/unittraits.ipp
    include/hatn/dataunit/detail/syntax.ipp
    include/hatn/dataunit/detail/types.ipp

    include/hatn/dataunit/fieldserialization.h

    include/hatn/dataunit/updatefilter.h
    include/hatn/dataunit/validator.h
    include/hatn/dataunit/fieldvalidators.h
    include/hatn/dataunit/stringvalidators.h
    include/hatn/dataunit/updatenotifier.h
    include/hatn/dataunit/unitvalidator.h

    include/hatn/dataunit/unitstrings.h
    include/hatn/dataunit/unitcontainer.h
    include/hatn/dataunit/unithandler.h

    include/hatn/dataunit/readunitfieldatpath.h
    include/hatn/dataunit/updateunitfieldatpath.h

    include/hatn/dataunit/prevalidatedupdate.h
)

SET (SOURCES
    src/syntax.cpp
    src/wiredata.cpp
    src/stream.cpp
    src/unit.cpp
    src/field.cpp
    src/fieldserialization.cpp
    src/unitstrings.cpp
)

BUILD_HATN_MODULE()