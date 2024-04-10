PROJECT(hatndataunit)

SET (HEADERS
    include/hatn/dataunit/dataunit.h
    include/hatn/dataunit/syntax.h
    include/hatn/dataunit/types.h
    include/hatn/dataunit/allocatorfactory.h
    include/hatn/dataunit/wiredata.h
    include/hatn/dataunit/stream.h
    include/hatn/dataunit/unit.h
    include/hatn/dataunit/fieldgetset.h
    include/hatn/dataunit/valuetypes.h
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
    include/hatn/dataunit/detail/fieldserialization.ipp

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

    include/hatn/dataunit/wirebuf.h
    include/hatn/dataunit/wirebufsolid.h
    include/hatn/dataunit/wirebufchained.h
    include/hatn/dataunit/detail/wirebuf.ipp

    include/hatn/dataunit/visitors.h

    include/hatn/dataunit/unitmeta.h
    include/hatn/dataunit/detail/unitmeta.ipp
    include/hatn/dataunit/unitmacros.h
    include/hatn/dataunit/datauniterror.h
    include/hatn/dataunit/datauniterrorcodes.h
)

SET (SOURCES
    src/syntax.cpp
    src/wiredata.cpp
    src/stream.cpp
    src/unit.cpp
    src/field.cpp
    src/unitstrings.cpp
    src/datauniterror.cpp
)

# check if string literals are suuported by compiler
CHECK_CXX_SOURCE_COMPILES("
        template <typename CharT, CharT ...s>
        constexpr auto operator\"\" _s() {
            return \"OK\";
        }

        int main()
        {
            const char* c=\"check\"_s;
            return 0;
        }
    "
    STRING_LITERALS_SUPPORTED)

IF (STRING_LITERALS_SUPPORTED)
    MESSAGE(STATUS "Enabling string literals support")
    SET (DATAUNIT_CONFIG_H_TEXT "${DATAUNIT_CONFIG_H_TEXT}\n#ifndef HATN_STRING_LITERAL \n    #define HATN_STRING_LITERAL\n#endif")
ELSE()
    MESSAGE(STATUS "Disabling string literals support")
ENDIF()

BUILD_HATN_MODULE()
