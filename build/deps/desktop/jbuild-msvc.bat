SET DEPS_UNIVERSAL_ROOT=C:\projects\hatn\deps
rem SET HATN_TREAT_WARNING_AS_ERROR=1
SET PREPARE_TESTS=1
rem SET HATN_TEST_NAME=TestPoolMemoryResource
rem SET CMAKE_CXX_STANDARD=14
rem set HATN_TEST_WRAP_C=1
SET BUILD_WORKERS=14

echo Start build at %TIME%
call ./hatn/build/lib/jenkins-args.bat all msvc x86_64 release shared "testplugin"
echo Finish build at %TIME%
