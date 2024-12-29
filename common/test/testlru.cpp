#include <iostream>
#include <chrono>

#include <boost/test/unit_test.hpp>

#include <hatn/common/cachelru.h>

#include <hatn/test/multithreadfixture.h>

HATN_USING
HATN_COMMON_USING
HATN_TEST_USING

namespace {

struct Item
{
    size_t id;

    Item(size_t id) : id(id)
    {}
};

using Capacity=std::integral_constant<size_t,4>;

}

BOOST_AUTO_TEST_SUITE(TestLru)

BOOST_FIXTURE_TEST_CASE(LruCache,MultiThreadFixture)
{
    using cacheT=CacheLru<size_t,Item,Capacity>;
    cacheT cache;

    BOOST_CHECK(cache.empty());
    BOOST_CHECK_EQUAL(cache.size(),0);
    BOOST_CHECK_EQUAL(cache.capacity(),Capacity::value);
    BOOST_CHECK(!cache.isFull());

    auto& item1=cache.emplaceItem(11,10);
    BOOST_CHECK(!cache.empty());
    BOOST_CHECK_EQUAL(cache.size(),1);
    BOOST_CHECK_EQUAL(cache.capacity(),Capacity::value);
    BOOST_CHECK(!cache.isFull());
    BOOST_CHECK_EQUAL(item1.id,10);
    BOOST_CHECK_EQUAL(item1.key(),11);
    auto& lruItem1=cache.lruItem();
    BOOST_CHECK_EQUAL(lruItem1.id,10);
    BOOST_CHECK_EQUAL(lruItem1.key(),11);
    auto& mruItem1=cache.mruItem();
    BOOST_CHECK_EQUAL(mruItem1.id,10);
    BOOST_CHECK_EQUAL(mruItem1.key(),11);

    auto& item2=cache.emplaceItem(22,20);
    BOOST_CHECK(!cache.empty());
    BOOST_CHECK_EQUAL(cache.size(),2);
    BOOST_CHECK(!cache.isFull());
    BOOST_CHECK_EQUAL(item2.id,20);
    BOOST_CHECK_EQUAL(item2.key(),22);
    auto& lruItem2=cache.lruItem();
    BOOST_CHECK_EQUAL(lruItem2.id,item1.id);
    BOOST_CHECK_EQUAL(lruItem2.key(),item1.key());
    auto& mruItem2=cache.mruItem();
    BOOST_CHECK_EQUAL(mruItem2.id,item2.id);
    BOOST_CHECK_EQUAL(mruItem2.key(),item2.key());

    auto& item3=cache.emplaceItem(33,30);
    BOOST_CHECK(!cache.empty());
    BOOST_CHECK_EQUAL(cache.size(),3);
    BOOST_CHECK(!cache.isFull());
    BOOST_CHECK_EQUAL(item3.id,30);
    BOOST_CHECK_EQUAL(item3.key(),33);
    auto& lruItem3=cache.lruItem();
    BOOST_CHECK_EQUAL(lruItem3.id,item1.id);
    BOOST_CHECK_EQUAL(lruItem3.key(),item1.key());
    auto& mruItem3=cache.mruItem();
    BOOST_CHECK_EQUAL(mruItem3.id,item3.id);
    BOOST_CHECK_EQUAL(mruItem3.key(),item3.key());

    auto& item4=cache.emplaceItem(44,40);
    BOOST_CHECK(!cache.empty());
    BOOST_CHECK_EQUAL(cache.size(),4);
    BOOST_CHECK(cache.isFull());
    BOOST_CHECK_EQUAL(item4.id,40);
    BOOST_CHECK_EQUAL(item4.key(),44);
    auto& lruItem4=cache.lruItem();
    BOOST_CHECK_EQUAL(lruItem4.id,item1.id);
    BOOST_CHECK_EQUAL(lruItem4.key(),item1.key());
    auto& mruItem4=cache.mruItem();
    BOOST_CHECK_EQUAL(mruItem4.id,item4.id);
    BOOST_CHECK_EQUAL(mruItem4.key(),item4.key());

    auto& item5=cache.emplaceItem(55,50);
    BOOST_CHECK(!cache.empty());
    BOOST_CHECK_EQUAL(cache.size(),4);
    BOOST_CHECK(cache.isFull());
    BOOST_CHECK_EQUAL(item5.id,50);
    BOOST_CHECK_EQUAL(item5.key(),55);
    auto& lruItem5=cache.lruItem();
    BOOST_CHECK_EQUAL(lruItem5.id,item2.id);
    BOOST_CHECK_EQUAL(lruItem5.key(),item2.key());
    auto& mruItem5=cache.mruItem();
    BOOST_CHECK_EQUAL(mruItem5.id,item5.id);
    BOOST_CHECK_EQUAL(mruItem5.key(),item5.key());

    cache.removeItem(item2);
    BOOST_CHECK(!cache.empty());
    BOOST_CHECK_EQUAL(cache.size(),3);
    BOOST_CHECK(!cache.isFull());
    auto& lruItem6=cache.lruItem();
    BOOST_CHECK_EQUAL(lruItem6.id,item3.id);
    BOOST_CHECK_EQUAL(lruItem6.key(),item3.key());
    auto& mruItem6=cache.mruItem();
    BOOST_CHECK_EQUAL(mruItem6.id,item5.id);
    BOOST_CHECK_EQUAL(mruItem6.key(),item5.key());

    size_t keyCount=0;
    size_t valueCount=0;
    auto each1=[&keyCount,&valueCount](cacheT::Item& item)
    {
        keyCount+=item.key();
        valueCount+=item.id;
        return true;
    };
    cache.each(each1);
    BOOST_CHECK_EQUAL(keyCount,item3.key()+item4.key()+item5.key());
    BOOST_CHECK_EQUAL(valueCount,item3.id+item4.id+item5.id);

    keyCount=0;
    valueCount=0;
    size_t i=0;
    auto each2=[&i,&keyCount,&valueCount](cacheT::Item& item)
    {
        keyCount+=item.key();
        valueCount+=item.id;
        i++;
        return i<2;
    };
    cache.each(each2);
    BOOST_CHECK_EQUAL(keyCount,item3.key()+item4.key());
    BOOST_CHECK_EQUAL(valueCount,item3.id+item4.id);

    cache.removeItem(item4);
    BOOST_CHECK(!cache.empty());
    BOOST_CHECK_EQUAL(cache.size(),2);
    BOOST_CHECK(!cache.isFull());
    auto& lruItem7=cache.lruItem();
    BOOST_CHECK_EQUAL(lruItem7.id,item3.id);
    BOOST_CHECK_EQUAL(lruItem7.key(),item3.key());
    auto& mruItem7=cache.mruItem();
    BOOST_CHECK_EQUAL(mruItem7.id,item5.id);
    BOOST_CHECK_EQUAL(mruItem7.key(),item5.key());

    auto& item8=cache.emplaceItem(88,80);
    BOOST_CHECK(!cache.empty());
    BOOST_CHECK_EQUAL(cache.size(),3);
    BOOST_CHECK(!cache.isFull());
    BOOST_CHECK_EQUAL(item8.id,80);
    BOOST_CHECK_EQUAL(item8.key(),88);
    auto& lruItem8=cache.lruItem();
    BOOST_CHECK_EQUAL(lruItem8.id,item3.id);
    BOOST_CHECK_EQUAL(lruItem8.key(),item3.key());
    auto& mruItem8=cache.mruItem();
    BOOST_CHECK_EQUAL(mruItem8.id,item8.id);
    BOOST_CHECK_EQUAL(mruItem8.key(),item8.key());

    cache.removeItem(item8);
    BOOST_CHECK(!cache.empty());
    BOOST_CHECK_EQUAL(cache.size(),2);
    BOOST_CHECK(!cache.isFull());
    auto& lruItem9=cache.lruItem();
    BOOST_CHECK_EQUAL(lruItem9.id,item3.id);
    BOOST_CHECK_EQUAL(lruItem9.key(),item3.key());
    auto& mruItem9=cache.mruItem();
    BOOST_CHECK_EQUAL(mruItem9.id,item5.id);
    BOOST_CHECK_EQUAL(mruItem9.key(),item5.key());
}

BOOST_AUTO_TEST_SUITE_END()
