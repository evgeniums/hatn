#include <boost/test/unit_test.hpp>

#include <hatn/common/simplequeue.h>

#include <hatn/test/multithreadfixture.h>

HATN_USING
HATN_COMMON_USING
HATN_TEST_USING

namespace {

struct TestStruct
{
    int val=0;
};

}

BOOST_AUTO_TEST_SUITE(TestSimpleQueue)

BOOST_AUTO_TEST_CASE(TestQueue)
{
    SimpleQueue<TestStruct> q;

    BOOST_CHECK(q.empty());
    BOOST_CHECK_EQUAL(q.size(),0);

    q.push(TestStruct{1});
    BOOST_CHECK(!q.empty());
    BOOST_CHECK_EQUAL(q.size(),1);

    auto v1=q.pop();
    BOOST_CHECK(q.empty());
    BOOST_CHECK_EQUAL(q.size(),0);
    BOOST_CHECK_EQUAL(v1.val,1);

    TestStruct v2{2};
    auto item2=q.prepare(v2);
    BOOST_CHECK(q.empty());
    BOOST_CHECK_EQUAL(q.size(),0);
    q.pushItem(item2);
    BOOST_CHECK(!q.empty());
    BOOST_CHECK_EQUAL(q.size(),1);
    QueueItem* item2q=nullptr;
    TestStruct* val2qPtr=nullptr;
    auto hasLastItem2=q.popValAndItem(val2qPtr,item2q);
    BOOST_CHECK(hasLastItem2);
    BOOST_CHECK(q.empty());
    BOOST_CHECK_EQUAL(q.size(),0);
    q.pushItem(item2q);
    BOOST_CHECK(!q.empty());
    BOOST_CHECK_EQUAL(q.size(),1);
    BOOST_CHECK_EQUAL(val2qPtr->val,2);

    auto item2p=q.popItem();
    BOOST_CHECK_EQUAL(item2p->m_val.val,2);
    BOOST_CHECK(q.empty());
    BOOST_CHECK_EQUAL(q.size(),0);
    q.pushItem(item2p);
    QueueItem* item2q_=nullptr;
    TestStruct* val2qPtr_=nullptr;
    auto hasLastItem2_=q.popValAndItem(val2qPtr_,item2q_);
    BOOST_CHECK(hasLastItem2_);
    BOOST_CHECK_EQUAL(val2qPtr_->val,2);

    q.clear();
    BOOST_CHECK(q.empty());
    BOOST_CHECK_EQUAL(q.size(),0);
}

BOOST_AUTO_TEST_SUITE_END()
