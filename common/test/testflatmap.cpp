#include <map>
#include <unordered_map>

#include <boost/test/unit_test.hpp>

#include <boost/container/flat_map.hpp>
#include <boost/intrusive/avl_set.hpp>

#include <hatn/common/elapsedtimer.h>
#include <hatn/common/flatmap.h>
#include <hatn/common/stdwrappers.h>

#include <hatn/test/multithreadfixture.h>

HATN_USING
HATN_COMMON_USING
HATN_TEST_USING

BOOST_AUTO_TEST_SUITE(TestFlatMap)

BOOST_AUTO_TEST_CASE(CheckFlatMapOps)
{
    FlatMap<size_t,size_t> fm;
    BOOST_CHECK(fm.empty());
    BOOST_CHECK_EQUAL(fm.size(),0);
    BOOST_CHECK_EQUAL(fm.capacity(),0);

    size_t count=8;

    fm.reserve(count);
    BOOST_CHECK_EQUAL(fm.capacity(),count);
    BOOST_CHECK(fm.empty());
    BOOST_CHECK_EQUAL(fm.size(),0);
    BOOST_CHECK_EQUAL(fm.capacity(),count);

    for (size_t i=0;i<count/2;i++)
    {
        fm.insert(std::make_pair(count-i,i+1));
    }
    for (size_t i=count-1;i>=count/2;i--)
    {
        fm.insert(std::make_pair(count-i,i+1));
    }
    BOOST_CHECK(!fm.empty());
    BOOST_CHECK_EQUAL(fm.size(),count);
    BOOST_CHECK_EQUAL(fm.capacity(),count);

    int ok=0;
    for (size_t i=0;i<count;i++)
    {
        auto it=fm.find(count-i);
        ok=(it!=fm.end())?0:1;
        if (ok!=0)
        {
            break;
        }
        ok=(it->first==(count-i))?0:2;
        if (ok!=0)
        {
            break;
        }
        ok=(it->second==(i+1))?0:3;
        if (ok!=0)
        {
            break;
        }
        ok=((*it).first==(count-i))?0:4;
        if (ok!=0)
        {
            break;
        }
        ok=((*it).second==(i+1))?0:5;
        if (ok!=0)
        {
            break;
        }
    }
    BOOST_CHECK_EQUAL(ok,0);

    ok=0;
    size_t iterCount=0;
    for (auto it=fm.begin();it!=fm.end();++it)
    {
        ok=(it->first==(iterCount+1))?0:1;
        if (ok!=0)
        {
            break;
        }
        ok=(it->second==(count-iterCount))?0:2;
        if (ok!=0)
        {
            break;
        }
        iterCount++;
    }
    BOOST_CHECK_EQUAL(ok,0);
    BOOST_CHECK_EQUAL(iterCount,count);

    auto res=fm.insert(std::make_pair(count/2,count*2));
    BOOST_CHECK(!res.second);
    BOOST_CHECK_EQUAL(fm.size(),count);
    auto it=fm.find(count/2);
    BOOST_REQUIRE(it!=fm.end());
    BOOST_CHECK_EQUAL(it->second,count*2);

    res=fm.insert_or_assign(std::make_pair(count/2,count*2+4));
    BOOST_CHECK(!res.second);
    BOOST_CHECK_EQUAL(fm.size(),count);
    it=fm.find(count/2);
    BOOST_REQUIRE(it!=fm.end());
    BOOST_CHECK_EQUAL(it->second,count*2+4);

    res=fm.insert_or_assign(std::make_pair(count+1,count*2+1));
    BOOST_CHECK(res.second);
    BOOST_CHECK_EQUAL(fm.size(),count+1);
    it=fm.find(count+1);
    BOOST_REQUIRE(it!=fm.end());
    BOOST_CHECK_EQUAL(it->second,count*2+1);

    const auto item=std::make_pair(count+2,count*2+2);
    res=fm.insert(item);
    BOOST_CHECK(res.second);
    BOOST_CHECK_EQUAL(fm.size(),count+2);
    auto it1=fm.find(count+2);
    BOOST_REQUIRE(it1!=fm.end());
    BOOST_CHECK_EQUAL(it1->second,count*2+2);

    auto it2=fm.erase(it);
    BOOST_CHECK_EQUAL(fm.size(),count+1);
    BOOST_REQUIRE(it2!=fm.end());
    BOOST_CHECK_EQUAL(it2->first,count+2);
    BOOST_CHECK_EQUAL(it2->second,count*2+2);

    auto it3=fm.erase(count+2);
    BOOST_CHECK(it3==fm.end());

    BOOST_CHECK(fm.capacity()!=fm.size());
    fm.shrinkToFit();
    BOOST_CHECK_EQUAL(fm.capacity(),fm.size());

    fm.beginRawInsert(count);
    for (size_t i=count*4-1;i>=count*3;i--)
    {
        fm.rawInsert(std::make_pair(i*10,i*100));
    }
    fm.endRawInsert();

    BOOST_CHECK_EQUAL(fm.size(),count+count);

    auto prevSize=fm.size();
    auto val=fm[1];
    BOOST_CHECK_EQUAL(val,count);
    BOOST_CHECK_EQUAL(fm.size(),prevSize);

    fm[0]=1234;
    auto val1=fm[0];
    BOOST_CHECK_EQUAL(val1,1234);
    auto idx1=1;
    fm[idx1]=4321;
    auto val2=fm[idx1];
    BOOST_CHECK_EQUAL(val2,4321);

    prevSize=fm.size();
    fm[count*5]=5678;
    auto it10=fm.find(count*5);
    BOOST_REQUIRE(it10!=fm.end());
    BOOST_CHECK_EQUAL(it10->second,5678);
    BOOST_CHECK_EQUAL(fm.size(),prevSize+1);

    prevSize=fm.size();
    auto idx2=count*5+1;
    fm[idx2]=8765;
    it10=fm.find(idx2);
    BOOST_REQUIRE(it10!=fm.end());
    BOOST_CHECK_EQUAL(it10->second,8765);
    BOOST_CHECK_EQUAL(fm.size(),prevSize+1);

    fm.clear();
    BOOST_CHECK(fm.empty());
    BOOST_CHECK_EQUAL(fm.size(),0);
    it10=fm.find(idx2);
    BOOST_REQUIRE(it10==fm.end());
}

BOOST_AUTO_TEST_CASE(CheckFlatMapComp)
{
    common::FlatMap<uint32_t,std::string,std::less<>> m1;
    m1[1]="a";

    auto it1=m1.find(1);
    BOOST_REQUIRE(it1!=m1.end());
    std::string v1=it1->second;
    BOOST_CHECK_EQUAL(v1,"a");

    const auto& m1_=m1;
    auto it1_=m1_.find(1);
    BOOST_REQUIRE(it1_!=m1_.end());
    std::string v1_=it1_->second;
    BOOST_CHECK_EQUAL(v1_,"a");

    common::FlatMap<std::string,uint32_t,std::less<>> m2;
    m2["a"]=1;

    lib::string_view s2("a");
    auto it2=m2.find(s2);
    BOOST_REQUIRE(it2!=m2.end());
    uint32_t v2=it2->second;
    BOOST_CHECK_EQUAL(v2,1);
}

// #define TEST_FLATMAP_PERFORMANCE

#ifdef TEST_FLATMAP_PERFORMANCE

namespace {

struct AvlItem : public boost::intrusive::avl_set_base_hook<>
{
    size_t key;
    size_t value;

    AvlItem(size_t key=0, size_t value=0):key(key),value(value)
    {}

    friend bool operator< (const AvlItem &a, const AvlItem &b) noexcept
         {  return a.key < b.key;  }
      friend bool operator> (const AvlItem &a, const AvlItem &b) noexcept
         {  return a.key > b.key;  }
      friend bool operator== (const AvlItem &a, const AvlItem &b) noexcept
         {  return a.key < b.key;  }
};

using AvlMap=boost::intrusive::avl_set<AvlItem,boost::intrusive::compare<std::less<AvlItem>>>;

}

BOOST_FIXTURE_TEST_CASE(CheckFlatMapPerformance,MultiThreadFixture, * boost::unit_test::disabled())
{
    srand(static_cast<unsigned int>(time(NULL)));

    ElapsedTimer elapsed;
    std::string elapsedStr;
    const size_t cycles=50000000;
    std::vector<size_t> sizes={3,5,10,20,30,100,1000,100000};
    for (auto size:sizes)
    {
        BOOST_TEST_MESSAGE(fmt::format("====Test items={}, cycles={} ====",size,cycles));

        std::vector<size_t> data(size);
        std::vector<size_t> keys(size);
        randSizeTVector(data);
        randSizeTVector(keys);

        std::vector<size_t> randSearchKeys(size);
        for (size_t i=0;i<size;i++)
        {
            auto idx=std::rand() % size;
            auto key=keys[idx];
            if (size<=10)
            {
                for (size_t j=0;j<randSearchKeys.size();j++)
                {
                    if (key==randSearchKeys[j])
                    {
                        idx=std::rand() % size;
                        key=keys[idx];
                    }
                }
            }
            randSearchKeys[i]=key;
        }

        {
            std::map<size_t,size_t> m;
            std::unordered_map<size_t,size_t> um;
            boost::container::flat_map<size_t,size_t> bm;
            FlatMap<size_t,size_t> fm;
            AvlMap am;

            BOOST_TEST_MESSAGE("Random insert");

            elapsed.reset();
            for (size_t k=0;k<cycles;k++)
            {
                auto idx=k%size;
                m[keys[idx]]=data[idx];
            }
            elapsedStr=elapsed.toString();
            BOOST_TEST_MESSAGE(fmt::format("std::map random insert {}",elapsedStr));

            // BOOST_TEST_MESSAGE("Begin std::unordered_map random insert");
            elapsed.reset();
            for (size_t k=0;k<cycles;k++)
            {
                auto idx=k%size;
                um[keys[idx]]=data[idx];
            }
            elapsedStr=elapsed.toString();
            BOOST_TEST_MESSAGE(fmt::format("std::unordered map random insert {}",elapsedStr));

            // BOOST_TEST_MESSAGE("Begin boost::flat_map random insert");
            elapsed.reset();
            bm.reserve(size);
            for (size_t k=0;k<cycles;k++)
            {
                auto idx=k%size;
                bm.insert_or_assign(keys[idx],data[idx]);
            }
            elapsedStr=elapsed.toString();
            BOOST_TEST_MESSAGE(fmt::format("boost::flat_map random insert {}",elapsedStr));

            // BOOST_TEST_MESSAGE("Begin boost::avl_set random insert");
            size_t sum=0;
            std::vector<AvlItem> avlItems(size);
            elapsed.reset();
            for (size_t k=0;k<cycles;k++)
            {
                auto idx=k%size;
                if (k<size)
                {
                    avlItems[idx]=AvlItem(keys[idx],data[idx]);
                }
                else
                {
                    sum+=keys[idx]+data[idx];
                }
                am.insert(avlItems[idx]);
            }
            elapsedStr=elapsed.toString();
            BOOST_TEST_MESSAGE(fmt::format("boost::avl_set random insert {}",elapsedStr));
            BOOST_CHECK(sum!=0);

            // BOOST_TEST_MESSAGE("Begin FlatMap random insert");
            elapsed.reset();
            fm.reserve(size);
            for (size_t k=0;k<cycles;k++)
            {
                auto idx=k%size;
                fm.insert_or_assign(std::make_pair(keys[idx],data[idx]));
            }
            elapsedStr=elapsed.toString();
            BOOST_TEST_MESSAGE(fmt::format("FlatMap random insert {}",elapsedStr));

            am.clear();
        }

        {
            std::map<size_t,size_t> m;
            boost::container::flat_map<size_t,size_t> bm;
            std::unordered_map<size_t,size_t> um;
            FlatMap<size_t,size_t> fm;
            AvlMap am;
            size_t sum=0;

            BOOST_TEST_MESSAGE("Begin search");

            for (size_t i=0;i<size;i++)
            {
                m[keys[i]]=data[i];
            }

            elapsed.reset();
            for (size_t i=0;i<cycles;i++)
            {
                auto idx=randSearchKeys[i%size];
                auto it=m.find(idx);
                if (it!=m.end())
                {
                    sum+=it->second;
                }
            }
            elapsedStr=elapsed.toString();
            BOOST_TEST_MESSAGE(fmt::format("std::map search {}",elapsedStr));
            BOOST_CHECK_GT(sum,0);

            // BOOST_TEST_MESSAGE("Begin std::unordered_map search");
            for (size_t i=0;i<size;i++)
            {
                um[keys[i]]=data[i];
            }
            sum=0;
            elapsed.reset();
            for (size_t i=0;i<cycles;i++)
            {
                auto idx=randSearchKeys[i%size];
                auto it=um.find(idx);
                if (it!=um.end())
                {
                    sum+=it->second;
                }
            }
            elapsedStr=elapsed.toString();
            BOOST_TEST_MESSAGE(fmt::format("std::unordered_map search {}",elapsedStr));
            BOOST_CHECK_GT(sum,0);

            // BOOST_TEST_MESSAGE("Begin boost::flat_map search");
            elapsed.reset();
            for (size_t i=0;i<size;i++)
            {
                bm.insert_or_assign(keys[i],data[i]);
            }
            sum=0;
            elapsed.reset();
            for (size_t i=0;i<cycles;i++)
            {
                auto idx=randSearchKeys[i%size];
                auto it=bm.find(idx);
                if (it!=bm.end())
                {
                    sum+=it->second;
                }
            }
            elapsedStr=elapsed.toString();
            BOOST_TEST_MESSAGE(fmt::format("boost::flat_map search {}",elapsedStr));
            BOOST_CHECK_GT(sum,0);

            // BOOST_TEST_MESSAGE("Begin boost::avl_set search");
            std::vector<AvlItem> avlItems(size);
            for (size_t i=0;i<size;i++)
            {
                avlItems[i]=AvlItem(keys[i],data[i]);
                am.insert(avlItems[i]);
            }
            sum=0;
            elapsed.reset();
            for (size_t i=0;i<cycles;i++)
            {
                auto idx=randSearchKeys[i%size];
                auto it=am.find(idx);
                if (it!=am.end())
                {
                    sum+=(*it).value;
                }
            }
            elapsedStr=elapsed.toString();
            am.clear();
            BOOST_TEST_MESSAGE(fmt::format("boost::avl_set search {}",elapsedStr));
            BOOST_CHECK_GT(sum,0);

            // BOOST_TEST_MESSAGE("Begin FlatMap search");
            for (size_t i=0;i<size;i++)
            {
                fm.insert_or_assign(std::make_pair(keys[i],data[i]));
            }
            sum=0;
            elapsed.reset();
            for (size_t i=0;i<cycles;i++)
            {
                auto idx=randSearchKeys[i%size];
                auto it=fm.find(idx);
                if (it!=fm.end())
                {
                    sum+=it->second;
                }
            }
            elapsedStr=elapsed.toString();
            BOOST_TEST_MESSAGE(fmt::format("FlatMap search {}",elapsedStr));
            BOOST_CHECK_GT(sum,0);
        }
    }

    BOOST_CHECK(true);
}
#endif

BOOST_AUTO_TEST_SUITE_END()
