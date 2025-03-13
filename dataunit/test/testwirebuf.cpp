#include <boost/test/unit_test.hpp>

#include <hatn/dataunit/wiredata.h>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#include <hatn/common/flatmap.h>

#include <hatn/common/pmr/withstaticallocator.h>
#include <hatn/common/pmr/withstaticallocator.ipp>

#include <hatn/dataunit/syntax.h>
#include "simpleunitdeclaration.h"

namespace du=HATN_DATAUNIT_NAMESPACE;
namespace common=HATN_COMMON_NAMESPACE;

BOOST_AUTO_TEST_SUITE(TestWireBuf)

BOOST_AUTO_TEST_CASE(WireBufBasicOp)
{
    du::WireBufSolid solid;
    du::WireBufSolidShared solidShared;
    du::WireBufChained chained;

    du::WireDataSingle vSolid;
    du::WireDataSingleShared vSolidShared;
    du::WireDataChained vChained;

    static_assert(du::WireBufSolid::isSingleBuffer(),"");
    static_assert(du::WireBufSolidShared::isSingleBuffer(),"");
    static_assert(!du::WireBufChained::isSingleBuffer(),"");
    BOOST_CHECK(vSolid.isSingleBuffer());
    BOOST_CHECK(vSolidShared.isSingleBuffer());
    BOOST_CHECK(!vChained.isSingleBuffer());

    auto size=solid.appendUint32(1122);
    BOOST_CHECK_EQUAL(2,size);

    size=solidShared.appendUint32(1122);
    BOOST_CHECK_EQUAL(2,size);

    size=chained.appendUint32(1122);
    BOOST_CHECK_EQUAL(2,size);

    size=vSolid.appendUint32(1122);
    BOOST_CHECK_EQUAL(2,size);

    size=vSolidShared.appendUint32(1122);
    BOOST_CHECK_EQUAL(2,size);

    size=vChained.appendUint32(1122);
    BOOST_CHECK_EQUAL(2,size);

    common::ByteArray solidB;
    du::copyToContainer(solid,&solidB);
    BOOST_CHECK_EQUAL(size,solidB.size());

    common::ByteArray b;
    du::copyToContainer(solidShared,&b);
    BOOST_CHECK(solidB==b);
    b.clear();
    du::copyToContainer(chained,&b);
    BOOST_CHECK(solidB==b);
    b.clear();
    du::copyToContainer(vSolid,&b);
    BOOST_CHECK(solidB==b);
    b.clear();
    du::copyToContainer(vSolidShared,&b);
    BOOST_CHECK(solidB==b);
    b.clear();
    du::copyToContainer(vChained,&b);
    BOOST_CHECK(solidB==b);

    auto oldSize=size;
    common::ByteArrayShared sharedBuf=common::makeShared<common::ByteArrayManaged>("Hello world!");

    solid.appendBuffer(sharedBuf);
    BOOST_CHECK_EQUAL(oldSize,solid.size());
    size=static_cast<int>(solid.mainContainer()->size());
    BOOST_CHECK_EQUAL(14,size);
    solidB.clear();
    du::copyToContainer(solid,&solidB);
    BOOST_CHECK_EQUAL(size,solidB.size());
    BOOST_CHECK(*solid.mainContainer()==solidB);

    solidShared.appendBuffer(sharedBuf);
    BOOST_CHECK_EQUAL(oldSize,solidShared.size());
    BOOST_CHECK_EQUAL(size,solidShared.mainContainer()->size());
    b.clear();
    du::copyToContainer(solidShared,&b);
    BOOST_CHECK(solidB==b);

    chained.appendBuffer(sharedBuf);
    BOOST_CHECK_EQUAL(oldSize,chained.size());
    BOOST_CHECK_EQUAL(1,chained.buffers().size());
    BOOST_CHECK_EQUAL(2,chained.chain().size());
    b.clear();
    du::copyToContainer(chained,&b);
    BOOST_CHECK(solidB==b);

    vSolid.appendBuffer(sharedBuf);
    BOOST_CHECK_EQUAL(oldSize,vSolid.size());
    BOOST_CHECK_EQUAL(size,vSolid.mainContainer()->size());
    b.clear();
    du::copyToContainer(vSolid,&b);
    BOOST_CHECK(solidB==b);

    vSolidShared.appendBuffer(sharedBuf);
    BOOST_CHECK_EQUAL(oldSize,vSolidShared.size());
    BOOST_CHECK_EQUAL(size,vSolidShared.mainContainer()->size());
    b.clear();
    du::copyToContainer(vSolidShared,&b);
    BOOST_CHECK(solidB==b);

    vChained.appendBuffer(sharedBuf);
    BOOST_CHECK_EQUAL(oldSize,vChained.size());
    BOOST_CHECK_EQUAL(1,vChained.buffers().size());
    BOOST_CHECK_EQUAL(2,vChained.chain().size());
    b.clear();
    du::copyToContainer(vChained,&b);
    BOOST_CHECK(solidB==b);
}

BOOST_AUTO_TEST_CASE(MsvcImportUnit)
{
    simple_int8::type du1;
    all_types::type du2;
    simple_int8::type du3;

    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()
