#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

#include <boost/test/unit_test.hpp>

#define HATN_TEST_SMART_POINTERS

#include <hatn/common/types.h>
#include <hatn/common/managedobject.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/weakptr.h>
#include <hatn/common/makeshared.h>
#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/common/pointers/mempool/weakptr.h>

#include <hatn/common/memorypool/newdeletepool.h>

#include <hatn/common/pmr/poolmemoryresource.h>
#include <hatn/common/pmr/uniqueptr.h>

#include <hatn/test/multithreadfixture.h>

HATN_USING
HATN_COMMON_USING
HATN_TEST_USING

struct TestStruct : public pointers_mempool::EnableSharedFromThis<TestStruct>
{
    uint32_t m_id;
    std::string m_str;
    char m_char;

    //! Ctor
    TestStruct(uint32_t id, const std::string& str=std::string(), char ch=char()):m_id(id),m_str(str),m_char(ch)
    {}
};

struct TestStructDerived : public TestStruct
{
    //! Ctor
    TestStructDerived(
            uint32_t id
        ) : TestStruct(id)
    {}
};

struct TestStructStd : public pointers_std::EnableSharedFromThis<TestStructStd>
{
    uint32_t m_id;
    std::string m_str;
    char m_char;

    //! Ctor
    TestStructStd(uint32_t id, const std::string& str=std::string(), char ch=char()):m_id(id),m_str(str),m_char(ch)
    {}
};
inline bool operator ==(const TestStructStd& left, const TestStructStd& right)
{
    return left.m_id==right.m_id && left.m_str==right.m_str && left.m_char==right.m_char;
}

struct TestStructDerivedStd : public TestStructStd
{
    uint32_t m_id1;

    //! Ctor
    TestStructDerivedStd(
            uint32_t id
        ) : TestStructStd(id),
	    m_id1(0)
    {}
};

struct PlainTestStruct
{
    uint32_t m_id;
    std::string m_str;
    char m_char;

    virtual ~PlainTestStruct()=default;
    PlainTestStruct(const PlainTestStruct&)=default;
    PlainTestStruct(PlainTestStruct&&) =default;
    PlainTestStruct& operator=(const PlainTestStruct&)=default;
    PlainTestStruct& operator=(PlainTestStruct&&) =default;

    PlainTestStruct(uint32_t id, const std::string& str=std::string(), char ch=char()):m_id(id),m_str(str),m_char(ch)
    {
    }
};

struct TestStlSharedDeleter : public pointers_std::ManagedObject
{
    int a=0;
};

struct TestStlSharedFromThis : public pointers_std::EnableSharedFromThis<TestStlSharedFromThis>
{
    int a=0;
};

BOOST_AUTO_TEST_SUITE(TestSmartPointers)

namespace {
template <typename T, typename T1> void MakeSharedImpl()
{
    auto obj=T::template makeShared<T1>(2525,"Hello world",'k');

    BOOST_CHECK_EQUAL(obj->m_id,static_cast<uint32_t>(2525));
    BOOST_CHECK_EQUAL(obj->m_str,std::string("Hello world"));
    BOOST_CHECK_EQUAL(obj->m_char,'k');

    typename T::template SharedPtr<T1> obj2;
    {
        auto obj1=T::template makeShared<T1>(5252,"Hi!",'q');
        obj2=obj1;

        BOOST_CHECK_EQUAL(obj1->m_id,static_cast<uint32_t>(5252));
        BOOST_CHECK_EQUAL(obj1->m_str,std::string("Hi!"));
        BOOST_CHECK_EQUAL(obj1->m_char,'q');
    }

    BOOST_CHECK_EQUAL(obj2->m_id,static_cast<uint32_t>(5252));
    BOOST_CHECK_EQUAL(obj2->m_str,std::string("Hi!"));
    BOOST_CHECK_EQUAL(obj2->m_char,'q');
    obj2.reset();
}
}

BOOST_FIXTURE_TEST_CASE(MakeSharedUnmanaged_MemPool,MultiThreadFixture)
{
    MakeSharedImpl<pointers_mempool::Pointers,PlainTestStruct>();
}

BOOST_FIXTURE_TEST_CASE(MakeSharedUnmanaged_Std,MultiThreadFixture)
{
    MakeSharedImpl<pointers_std::Pointers,PlainTestStruct>();
}

BOOST_FIXTURE_TEST_CASE(MakeSharedManaged_MemPool,MultiThreadFixture)
{
    MakeSharedImpl<pointers_mempool::Pointers,TestStruct>();
}

BOOST_FIXTURE_TEST_CASE(MakeSharedManaged_Std,MultiThreadFixture)
{
    MakeSharedImpl<pointers_std::Pointers,TestStructStd>();
}

namespace {
template <typename T, typename T1> void AllocateSharedImpl(pmr::memory_resource* memoryResource)
{
    pmr::polymorphic_allocator<T1> allocator(memoryResource);

    auto obj=T::template allocateShared<T1>(allocator,2525,"Hello world",'k');

    BOOST_CHECK_EQUAL(obj->m_id,static_cast<uint32_t>(2525));
    BOOST_CHECK_EQUAL(obj->m_str,std::string("Hello world"));
    BOOST_CHECK_EQUAL(obj->m_char,'k');

    typename T::template SharedPtr<T1> obj2;
    {
        auto obj1=T::template allocateShared<T1>(allocator,5252,"Hi!",'q');
        obj2=obj1;

        BOOST_CHECK_EQUAL(obj1->m_id,static_cast<uint32_t>(5252));
        BOOST_CHECK_EQUAL(obj1->m_str,std::string("Hi!"));
        BOOST_CHECK_EQUAL(obj1->m_char,'q');
    }

    BOOST_CHECK_EQUAL(obj2->m_id,static_cast<uint32_t>(5252));
    BOOST_CHECK_EQUAL(obj2->m_str,std::string("Hi!"));
    BOOST_CHECK_EQUAL(obj2->m_char,'q');
    obj2.reset();
}
}

BOOST_FIXTURE_TEST_CASE(AllocateSharedUnmanagedDefaultResource_MemPool,MultiThreadFixture)
{
    AllocateSharedImpl<pointers_mempool::Pointers,PlainTestStruct>(pmr::get_default_resource());
}
BOOST_FIXTURE_TEST_CASE(AllocateSharedUnmanagedDefaultResource_Std,MultiThreadFixture)
{
    AllocateSharedImpl<pointers_std::Pointers,PlainTestStruct>(pmr::get_default_resource());
}
BOOST_FIXTURE_TEST_CASE(AllocateSharedManagedDefaultResource_MemPool,MultiThreadFixture)
{
    AllocateSharedImpl<pointers_mempool::Pointers,TestStruct>(pmr::get_default_resource());
}
BOOST_FIXTURE_TEST_CASE(AllocateSharedManagedDefaultResource_Std,MultiThreadFixture)
{
    AllocateSharedImpl<pointers_std::Pointers,TestStructStd>(pmr::get_default_resource());
}

namespace {

using MemoryResource=memorypool::NewDeletePoolResource;
using MemoryPool=memorypool::NewDeletePool;

struct MemResourceConfig
{
    MemResourceConfig(const std::shared_ptr<memorypool::PoolCacheGen<MemoryPool>>& poolCacheGen):opts(poolCacheGen)
    {
    }

    MemoryResource::Options opts;
};

std::unique_ptr<pmr::memory_resource> makeResource(const MemResourceConfig& config)
{
    return std::make_unique<MemoryResource>(config.opts);
}
}

BOOST_FIXTURE_TEST_CASE(AllocateSharedUnmanagedPoolResource_MemPool,MultiThreadFixture)
{
    MemResourceConfig config(poolCacheGen<MemoryPool>());
    auto resource=makeResource(config);

    AllocateSharedImpl<pointers_mempool::Pointers,PlainTestStruct>(resource.get());
}
BOOST_FIXTURE_TEST_CASE(AllocateSharedUnmanagedPoolResource_Std,MultiThreadFixture)
{
    MemResourceConfig config(poolCacheGen<MemoryPool>());
    auto resource=makeResource(config);

    AllocateSharedImpl<pointers_std::Pointers,PlainTestStruct>(resource.get());
}
BOOST_FIXTURE_TEST_CASE(AllocateSharedManagedPoolResource_MemPool,MultiThreadFixture)
{
    MemResourceConfig config(poolCacheGen<MemoryPool>());
    auto resource=makeResource(config);

    AllocateSharedImpl<pointers_mempool::Pointers,TestStruct>(resource.get());
}
BOOST_FIXTURE_TEST_CASE(AllocateSharedManagedPoolResource_Std,MultiThreadFixture)
{
    MemResourceConfig config(poolCacheGen<MemoryPool>());
    auto resource=makeResource(config);

    AllocateSharedImpl<pointers_std::Pointers,TestStructStd>(resource.get());
}

namespace {
template <typename T, typename T1, typename T2> void CastingImpl(pmr::memory_resource* memoryResource)
{
    pmr::polymorphic_allocator<T1> allocator(memoryResource);

    auto ptr1=T::template allocateShared<T1>(allocator,500);
    auto ptr2=ptr1.template staticCast<typename T::ManagedObject>();
    BOOST_CHECK_EQUAL(ptr1.refCount(),2);

    auto ptr3=ptr2.template dynamicCast<T1>();
    BOOST_CHECK_EQUAL(ptr3.refCount(),3);

    ptr2.reset();
    BOOST_CHECK_EQUAL(ptr1.refCount(),2);

    typename T::template WeakPtr<T2> wPtr1=ptr3.template staticCast<T2>();
    auto ptr4=wPtr1.lock();
    BOOST_CHECK(!ptr4.isNull());
    BOOST_CHECK_EQUAL(ptr1.refCount(),3);

    typename T::template WeakPtr<typename T::ManagedObject> wPtr2;
    wPtr2=ptr3.template staticCast<typename T::ManagedObject>();
    auto ptr5=wPtr2.lock();
    BOOST_CHECK(!ptr5.isNull());
    BOOST_CHECK_EQUAL(ptr1.refCount(),4);
}
}

BOOST_FIXTURE_TEST_CASE(Cast_MemPool,MultiThreadFixture)
{
    MemResourceConfig config(poolCacheGen<MemoryPool>());
    auto resource=makeResource(config);

    CastingImpl<pointers_mempool::Pointers,TestStructDerived,TestStruct>(resource.get());
}
BOOST_FIXTURE_TEST_CASE(Cast_Std,MultiThreadFixture)
{
    MemResourceConfig config(poolCacheGen<MemoryPool>());
    auto resource=makeResource(config);

    CastingImpl<pointers_std::Pointers,TestStructDerivedStd,TestStructStd>(resource.get());
}

namespace {

template <typename T,typename T1,typename=void> struct Ctor
{};
template <typename T,typename T1> struct Ctor<
        T,
        T1,
        std::enable_if_t<!std::is_same<T,pointers_std::Pointers>::value>
    >
{
    static typename T::template SharedPtr<T1> f(T1* obj, pmr::polymorphic_allocator<T1>& allocator)
    {
        return typename T::template SharedPtr<T1>(obj,obj,allocator.resource());
    }

    static void r(typename T::template SharedPtr<T1>& ptr,T1* obj, pmr::polymorphic_allocator<T1>& allocator)
    {
        ptr.reset(obj,obj,allocator.resource());
    }
};
template <typename T,typename T1> struct Ctor<
        T,
        T1,
        std::enable_if_t<std::is_same<T,pointers_std::Pointers>::value>
    >
{
    static typename T::template SharedPtr<T1> f(T1* obj, pmr::polymorphic_allocator<T1>& allocator)
    {
        obj->setMemoryResource(allocator.resource());
        return typename T::template SharedPtr<T1>(obj);
    }

    static void r(typename T::template SharedPtr<T1>& ptr,T1* obj, pmr::polymorphic_allocator<T1>& allocator)
    {
        obj->setMemoryResource(allocator.resource());
        ptr.reset(obj);
    }
};

template <typename T, typename T1> void SharedManagedImpl(pmr::memory_resource* memoryResource)
{
    pmr::polymorphic_allocator<T1> allocator(memoryResource);

    auto* obj=allocator.allocate(1);
    allocator.construct(obj,300);

    typename T::template SharedPtr<T1> ptr=Ctor<T,T1>::f(obj,allocator);
    BOOST_CHECK_EQUAL(ptr->m_id,static_cast<uint32_t>(300));
    BOOST_CHECK_EQUAL(ptr.refCount(),1);
    BOOST_CHECK(!ptr.isNull());
    BOOST_CHECK_EQUAL(ptr.get(),obj);
    BOOST_CHECK(*ptr==*obj);
    ptr->m_id=500;
    BOOST_CHECK_EQUAL(ptr->m_id,static_cast<uint32_t>(500));
    {
        auto newPtr=ptr;
        BOOST_CHECK_EQUAL(ptr.refCount(),2);
        BOOST_CHECK_EQUAL(newPtr.refCount(),2);
        BOOST_CHECK_EQUAL(newPtr->m_id,static_cast<uint32_t>(500));

        newPtr->m_id=700;
        BOOST_CHECK_EQUAL(ptr->m_id,static_cast<uint32_t>(700));

        BOOST_CHECK(!newPtr.isNull());
        BOOST_CHECK_EQUAL(newPtr.get(),obj);
        BOOST_CHECK(*newPtr==*obj);

        {
            auto fromThisPtr=ptr->sharedFromThis();
            BOOST_CHECK_EQUAL(ptr.refCount(),3);
            BOOST_CHECK_EQUAL(newPtr.refCount(),3);
            BOOST_CHECK_EQUAL(fromThisPtr.refCount(),3);
            BOOST_CHECK_EQUAL(fromThisPtr->m_id,static_cast<uint32_t>(700));

            fromThisPtr->m_id=7900;
            BOOST_CHECK_EQUAL(ptr->m_id,static_cast<uint32_t>(7900));

            BOOST_CHECK(!fromThisPtr.isNull());
            BOOST_CHECK_EQUAL(fromThisPtr.get(),obj);
            BOOST_CHECK(*fromThisPtr==*obj);
        }

        {
            typename T::template SharedPtr<T1> nextPtr;
            nextPtr=newPtr;
            BOOST_CHECK_EQUAL(nextPtr->m_id,static_cast<uint32_t>(7900));

            newPtr->m_id=900;
            BOOST_CHECK_EQUAL(nextPtr->m_id,static_cast<uint32_t>(900));

            nextPtr->m_id=1000;
            BOOST_CHECK_EQUAL(nextPtr->m_id,static_cast<uint32_t>(1000));
            BOOST_CHECK_EQUAL(newPtr->m_id,static_cast<uint32_t>(1000));
            BOOST_CHECK_EQUAL(ptr->m_id,static_cast<uint32_t>(1000));

            BOOST_CHECK_EQUAL(ptr.refCount(),3);
            BOOST_CHECK_EQUAL(newPtr.refCount(),3);
            BOOST_CHECK_EQUAL(nextPtr.refCount(),3);

            BOOST_CHECK(!nextPtr.isNull());
            BOOST_CHECK_EQUAL(nextPtr.get(),obj);
            BOOST_CHECK(*nextPtr==*obj);

            auto* nextObj=allocator.allocate(1);
            allocator.construct(nextObj,50);
            Ctor<T,T1>::r(nextPtr,nextObj,allocator);

            BOOST_CHECK_EQUAL(ptr.refCount(),2);
            BOOST_CHECK_EQUAL(newPtr.refCount(),2);

            BOOST_CHECK_EQUAL(nextPtr.refCount(),1);
            BOOST_CHECK(!nextPtr.isNull());
            BOOST_CHECK_EQUAL(nextPtr.get(),nextObj);
            BOOST_CHECK(*nextPtr==*nextObj);

            auto nextPtr1=T::template allocateShared<T1>(allocator,20);
            BOOST_CHECK_EQUAL(nextPtr1.refCount(),1);
            BOOST_CHECK(!nextPtr1.isNull());
        }
        BOOST_CHECK_EQUAL(newPtr->m_id,static_cast<uint32_t>(1000));
        BOOST_CHECK_EQUAL(ptr->m_id,static_cast<uint32_t>(1000));

        {
            typename T::template SharedPtr<typename T::ManagedObject> nextPtr(obj->sharedFromThis());

            BOOST_CHECK(!nextPtr.isNull());
            BOOST_CHECK_EQUAL(nextPtr.get(),obj);

            BOOST_CHECK_EQUAL(ptr.refCount(),3);
            BOOST_CHECK_EQUAL(newPtr.refCount(),3);
            BOOST_CHECK_EQUAL(nextPtr.refCount(),3);

            BOOST_CHECK(!nextPtr.isNull());
            BOOST_CHECK_EQUAL(nextPtr.get(),obj);
        }
    }
    BOOST_CHECK_EQUAL(ptr.refCount(),1);
    BOOST_CHECK_EQUAL(ptr->m_id,static_cast<uint32_t>(1000));

    ptr.reset();

    BOOST_CHECK_EQUAL(ptr.refCount(),0);
    BOOST_CHECK(ptr.isNull());
    BOOST_CHECK(ptr.get()==nullptr);
}
}

BOOST_FIXTURE_TEST_CASE(SharedManagedPoolResource_MemPool,MultiThreadFixture)
{
    MemResourceConfig config(poolCacheGen<MemoryPool>());
    auto resource=makeResource(config);

    SharedManagedImpl<pointers_mempool::Pointers,TestStruct>(resource.get());
}
BOOST_FIXTURE_TEST_CASE(SharedManagedPoolResource_Std,MultiThreadFixture)
{
    MemResourceConfig config(poolCacheGen<MemoryPool>());
    auto resource=makeResource(config);

    SharedManagedImpl<pointers_std::Pointers,TestStructStd>(resource.get());
}
BOOST_FIXTURE_TEST_CASE(SharedManagedDefaultResource_MemPool,MultiThreadFixture)
{
    SharedManagedImpl<pointers_mempool::Pointers,TestStruct>(pmr::get_default_resource());
}
BOOST_FIXTURE_TEST_CASE(SharedManagedDefaultResource_Std,MultiThreadFixture)
{
    SharedManagedImpl<pointers_std::Pointers,TestStructStd>(pmr::get_default_resource());
}

namespace {

template <typename T, typename T1> void WeakMemPoolImpl(pmr::memory_resource* memoryResource)
{
    pmr::polymorphic_allocator<T1> allocator(memoryResource);

    typename T::template WeakPtr<T1> wPtr2;

    BOOST_CHECK(wPtr2.isNull());
    {
        auto ptr1=T::template allocateShared<T1>(allocator,100);
        BOOST_CHECK(ptr1.weakCtrl()==nullptr);
        BOOST_CHECK(ptr1->weakCtrl()==nullptr);

        typename T::template WeakPtr<T1> wPtr1(ptr1);
        BOOST_CHECK(ptr1.weakCtrl()!=nullptr);
        BOOST_CHECK(ptr1->weakCtrl()!=nullptr);
        BOOST_CHECK(ptr1->weakCtrl()==ptr1.weakCtrl());
        BOOST_CHECK(!wPtr1.isNull());

        {
            typename T::template SharedPtr<T1> ptr2=wPtr1.lock();
            BOOST_REQUIRE(!ptr2.isNull());
            BOOST_CHECK_EQUAL(ptr2->m_id,static_cast<uint32_t>(100));
            BOOST_CHECK_EQUAL(ptr2.refCount(),static_cast<size_t>(2));

            wPtr2=ptr1;
            BOOST_CHECK(!wPtr2.isNull());

            typename T::template SharedPtr<T1> ptr3=wPtr2.lock();
            BOOST_CHECK(!ptr3.isNull());
            BOOST_CHECK_EQUAL(ptr3->m_id,static_cast<size_t>(100));
            BOOST_CHECK_EQUAL(ptr1.refCount(),3);
        }

        typename T::template SharedPtr<T1> ptr5=wPtr1.lock();
        BOOST_CHECK(!ptr5.isNull());
        BOOST_CHECK_EQUAL(ptr5->m_id,static_cast<uint32_t>(100));
        BOOST_CHECK_EQUAL(ptr5.refCount(),2);

        auto&& ptr7=wPtr1.lock();
        BOOST_CHECK(!ptr7.isNull());
        BOOST_CHECK_EQUAL(ptr7->m_id,static_cast<uint32_t>(100));
        BOOST_CHECK_EQUAL(ptr7.refCount(),3);

        BOOST_CHECK(wPtr1.ctrl()->sharedObject()!=nullptr);

        ptr7.reset();
        ptr5.reset();

        BOOST_CHECK(wPtr2.ctrl()->sharedObject()!=nullptr);
        ptr1.reset();

        BOOST_CHECK(wPtr1.ctrl()->sharedObject()==nullptr);

        typename T::template SharedPtr<T1> ptr6=wPtr1.lock();
        BOOST_CHECK(ptr6.isNull());
    }

    BOOST_CHECK(wPtr2.isNull());
    BOOST_CHECK(wPtr2.ctrl()->sharedObject()==nullptr);
    auto ptr4=wPtr2.lock();
    BOOST_CHECK(ptr4.isNull());
    wPtr2.reset();
    BOOST_CHECK(wPtr2.isNull());
}

template <typename T, typename T1> void WeakStdImpl(pmr::memory_resource* memoryResource)
{
    pmr::polymorphic_allocator<T1> allocator(memoryResource);

    typename T::template WeakPtr<T1> wPtr2;

    BOOST_CHECK(wPtr2.isNull());
    {
        auto ptr1=T::template allocateShared<T1>(allocator,100);

        typename T::template WeakPtr<T1> wPtr1(ptr1);
        BOOST_CHECK(!wPtr1.isNull());

        {
            typename T::template SharedPtr<T1> ptr2=wPtr1.lock();
            BOOST_REQUIRE(!ptr2.isNull());
            BOOST_CHECK_EQUAL(ptr2->m_id,static_cast<uint32_t>(100));
            BOOST_CHECK_EQUAL(ptr2.refCount(),static_cast<size_t>(2));

            wPtr2=ptr1;
            BOOST_CHECK(!wPtr2.isNull());

            typename T::template SharedPtr<T1> ptr3=wPtr2.lock();
            BOOST_CHECK(!ptr3.isNull());
            BOOST_CHECK_EQUAL(ptr3->m_id,static_cast<size_t>(100));
            BOOST_CHECK_EQUAL(ptr1.refCount(),3);
        }

        typename T::template SharedPtr<T1> ptr5=wPtr1.lock();
        BOOST_CHECK(!ptr5.isNull());
        BOOST_CHECK_EQUAL(ptr5->m_id,static_cast<uint32_t>(100));
        BOOST_CHECK_EQUAL(ptr5.refCount(),2);

        auto&& ptr7=wPtr1.lock();
        BOOST_CHECK(!ptr7.isNull());
        BOOST_CHECK_EQUAL(ptr7->m_id,static_cast<uint32_t>(100));
        BOOST_CHECK_EQUAL(ptr7.refCount(),3);

        ptr7.reset();
        ptr5.reset();

        ptr1.reset();

        typename T::template SharedPtr<T1> ptr6=wPtr1.lock();
        BOOST_CHECK(ptr6.isNull());
    }

    BOOST_CHECK(wPtr2.isNull());
    auto ptr4=wPtr2.lock();
    BOOST_CHECK(ptr4.isNull());
    wPtr2.reset();
    BOOST_CHECK(wPtr2.isNull());
}

}

BOOST_FIXTURE_TEST_CASE(WeakManagedDefaultResource_MemPool,MultiThreadFixture)
{
    WeakMemPoolImpl<pointers_mempool::Pointers,TestStruct>(pmr::get_default_resource());
}
BOOST_FIXTURE_TEST_CASE(WeakManagedDefaultResource_Std,MultiThreadFixture)
{
    WeakStdImpl<pointers_std::Pointers,TestStructStd>(pmr::get_default_resource());
}
BOOST_FIXTURE_TEST_CASE(WeakManagedPoolResource_MemPool,MultiThreadFixture)
{
    MemResourceConfig config(poolCacheGen<MemoryPool>());
    auto resource=makeResource(config);

    WeakMemPoolImpl<pointers_mempool::Pointers,TestStruct>(resource.get());
}
BOOST_FIXTURE_TEST_CASE(WeakManagedPoolResource_Std,MultiThreadFixture)
{
    MemResourceConfig config(poolCacheGen<MemoryPool>());
    auto resource=makeResource(config);

    WeakStdImpl<pointers_std::Pointers,TestStructStd>(resource.get());
}

BOOST_FIXTURE_TEST_CASE(WeakPlainDefaultResource_MemPool,MultiThreadFixture)
{
    WeakStdImpl<pointers_mempool::Pointers,PlainTestStruct>(pmr::get_default_resource());
}
BOOST_FIXTURE_TEST_CASE(WeakPlainDefaultResource_Std,MultiThreadFixture)
{
    WeakStdImpl<pointers_std::Pointers,PlainTestStruct>(pmr::get_default_resource());
}
BOOST_FIXTURE_TEST_CASE(WeakPlainPoolResource_MemPool,MultiThreadFixture)
{
    MemResourceConfig config(poolCacheGen<MemoryPool>());
    auto resource=makeResource(config);

    WeakStdImpl<pointers_mempool::Pointers,PlainTestStruct>(resource.get());
}
BOOST_FIXTURE_TEST_CASE(WeakPlainPoolResource_Std,MultiThreadFixture)
{
    MemResourceConfig config(poolCacheGen<MemoryPool>());
    auto resource=makeResource(config);

    WeakStdImpl<pointers_std::Pointers,PlainTestStruct>(resource.get());
}

struct PlainDelete
{
    static int DelCount;
    PlainDelete(int a=0):aaa(a)
    {}
    ~PlainDelete()
    {
        ++DelCount;
    }
    PlainDelete(const PlainDelete&)=default;
    PlainDelete(PlainDelete&&) =default;
    PlainDelete& operator=(const PlainDelete&)=default;
    PlainDelete& operator=(PlainDelete&&) =default;

    int aaa=0;
};
int PlainDelete::DelCount=0;

struct ManagedDeleteStd : public pointers_std::EnableManaged<ManagedDeleteStd>
{
    static int DelCount;
    ManagedDeleteStd(int a=0):aaa(a)
    {}
    ~ManagedDeleteStd()
    {
        ++DelCount;
    }
    ManagedDeleteStd(const ManagedDeleteStd&)=delete;
    ManagedDeleteStd(ManagedDeleteStd&&) =delete;
    ManagedDeleteStd& operator=(const ManagedDeleteStd&)=delete;
    ManagedDeleteStd& operator=(ManagedDeleteStd&&) =delete;
    int aaa=0;
};
int ManagedDeleteStd::DelCount=0;

BOOST_FIXTURE_TEST_CASE(StdDeleter,MultiThreadFixture)
{
    PlainDelete::DelCount=0;

    MemResourceConfig config(poolCacheGen<MemoryPool>());
    auto resource=makeResource(config);

    {
        pointers_std::Pointers::SharedPtr<PlainDelete> p(new PlainDelete);
        BOOST_CHECK(p);
        BOOST_CHECK_EQUAL(p->aaa,0);
    }
    BOOST_CHECK_EQUAL(PlainDelete::DelCount,1);
    {
        auto p=pointers_std::Pointers::makeShared<PlainDelete>();
        BOOST_CHECK(p);
        BOOST_CHECK_EQUAL(p->aaa,0);
    }
    BOOST_CHECK_EQUAL(PlainDelete::DelCount,2);
    pmr::polymorphic_allocator<PlainDelete> a1(resource.get());
    {
        auto p=pointers_std::Pointers::allocateShared(a1,10);
        BOOST_CHECK(p);
        BOOST_CHECK_EQUAL(p->aaa,10);
    }
    BOOST_CHECK_EQUAL(PlainDelete::DelCount,3);

    pmr::polymorphic_allocator<ManagedDeleteStd> a2(resource.get());
    {
        auto obj=a2.allocate(1);
        a2.construct(obj,20);
        obj->setMemoryResource(a2.resource());

        pointers_std::Pointers::SharedPtr<ManagedDeleteStd> p(obj);
        BOOST_CHECK(p);
        BOOST_CHECK_EQUAL(p->aaa,20);
    }
    BOOST_CHECK_EQUAL(ManagedDeleteStd::DelCount,1);
    {
        auto p=pointers_std::Pointers::makeShared<ManagedDeleteStd>();
        BOOST_CHECK(p);
        BOOST_CHECK_EQUAL(p->aaa,0);
    }
    BOOST_CHECK_EQUAL(ManagedDeleteStd::DelCount,2);
    {
        auto p=pointers_std::Pointers::allocateShared(a2,10);
        BOOST_CHECK(p);
        BOOST_CHECK_EQUAL(p->aaa,10);
    }
    BOOST_CHECK_EQUAL(ManagedDeleteStd::DelCount,3);

    {
        pointers_std::Pointers::SharedPtr<ManagedDeleteStd> p(new ManagedDeleteStd(30));
        BOOST_CHECK(p);
        BOOST_CHECK_EQUAL(p->aaa,30);
    }
    BOOST_CHECK_EQUAL(ManagedDeleteStd::DelCount,4);
}

struct ManagedDeleteMP : public pointers_mempool::EnableManaged<ManagedDeleteMP>
{
    static int DelCount;
    ManagedDeleteMP(int a=0):aaa(a)
    {}
    ~ManagedDeleteMP()
    {
        ++DelCount;
    }
    ManagedDeleteMP(const ManagedDeleteMP&)=delete;
    ManagedDeleteMP(ManagedDeleteMP&&) =delete;
    ManagedDeleteMP& operator=(const ManagedDeleteMP&)=delete;
    ManagedDeleteMP& operator=(ManagedDeleteMP&&) =delete;

    int aaa=0;
};
int ManagedDeleteMP::DelCount=0;

BOOST_FIXTURE_TEST_CASE(MempoolDeleter,MultiThreadFixture)
{
    PlainDelete::DelCount=0;

    MemResourceConfig config(poolCacheGen<MemoryPool>());
    auto resource=makeResource(config);

    {
        pointers_std::Pointers::SharedPtr<PlainDelete> p(new PlainDelete);
        BOOST_CHECK(p);
        BOOST_CHECK_EQUAL(p->aaa,0);
    }
    BOOST_CHECK_EQUAL(PlainDelete::DelCount,1);
    {
        auto p=pointers_std::Pointers::makeShared<PlainDelete>();
        BOOST_CHECK(p);
        BOOST_CHECK_EQUAL(p->aaa,0);
    }
    BOOST_CHECK_EQUAL(PlainDelete::DelCount,2);
    pmr::polymorphic_allocator<PlainDelete> a1(resource.get());
    {
        auto p=pointers_std::Pointers::allocateShared(a1,10);
        BOOST_CHECK(p);
        BOOST_CHECK_EQUAL(p->aaa,10);
    }
    BOOST_CHECK_EQUAL(PlainDelete::DelCount,3);

    pmr::polymorphic_allocator<ManagedDeleteMP> a2(resource.get());
    {
        auto obj=a2.allocate(1);
        a2.construct(obj,20);
        obj->setMemoryResource(a2.resource());

        pointers_mempool::Pointers::SharedPtr<ManagedDeleteMP> p(obj);
        BOOST_CHECK(p);
        BOOST_CHECK_EQUAL(p->aaa,20);
    }
    BOOST_CHECK_EQUAL(ManagedDeleteMP::DelCount,1);
    {
        auto p=pointers_mempool::Pointers::makeShared<ManagedDeleteMP>();
        BOOST_CHECK(p);
        BOOST_CHECK_EQUAL(p->aaa,0);
    }
    BOOST_CHECK_EQUAL(ManagedDeleteMP::DelCount,2);
    {
        auto p=pointers_mempool::Pointers::allocateShared(a2,10);
        BOOST_CHECK(p);
        BOOST_CHECK_EQUAL(p->aaa,10);
    }
    BOOST_CHECK_EQUAL(ManagedDeleteMP::DelCount,3);

    {
        pointers_mempool::Pointers::SharedPtr<ManagedDeleteMP> p(new ManagedDeleteMP(30));
        BOOST_CHECK(p);
        BOOST_CHECK_EQUAL(p->aaa,30);
    }
    BOOST_CHECK_EQUAL(ManagedDeleteMP::DelCount,4);
}

BOOST_FIXTURE_TEST_CASE(StaticCast,MultiThreadFixture)
{
    SharedPtr<TestStruct> obj1;
    BOOST_CHECK_EQUAL(obj1.refCount(),0);
    {
        auto obj=pmr::AllocatorFactory::getDefault()->createObject<TestStructDerived>(std::forward<uint32_t>(10));
        BOOST_CHECK_EQUAL(obj.refCount(),1);
        BOOST_CHECK_EQUAL(obj->m_id,10);
        obj1=obj.template staticCast<TestStruct>();
        BOOST_CHECK_EQUAL(obj1.refCount(),2);
        BOOST_CHECK_EQUAL(obj.refCount(),2);
        BOOST_CHECK_EQUAL(obj->m_id,10);
        BOOST_CHECK_EQUAL(obj1->m_id,10);
    }
    BOOST_CHECK_EQUAL(obj1.refCount(),1);
    BOOST_CHECK_EQUAL(obj1->m_id,10);
    obj1.reset();
    BOOST_CHECK_EQUAL(obj1.refCount(),0);
}

using PlainTestStructManaged=ManagedWrapper<PlainTestStruct>;

BOOST_FIXTURE_TEST_CASE(CastManagedToPlain,MultiThreadFixture)
{
    SharedPtr<PlainTestStruct> obj1;
    BOOST_CHECK_EQUAL(obj1.refCount(),0);
    {
        auto obj=pmr::AllocatorFactory::getDefault()->createObject<PlainTestStructManaged>(std::forward<uint32_t>(10));
        BOOST_CHECK_EQUAL(obj.refCount(),1);
        BOOST_CHECK_EQUAL(obj->m_id,10);
        obj1=obj.template staticCast<PlainTestStruct>();
        BOOST_CHECK_EQUAL(obj1.refCount(),2);
        BOOST_CHECK_EQUAL(obj.refCount(),2);
        BOOST_CHECK_EQUAL(obj->m_id,10);
        BOOST_CHECK_EQUAL(obj1->m_id,10);
    }
    BOOST_CHECK_EQUAL(obj1.refCount(),1);
    BOOST_CHECK_EQUAL(obj1->m_id,10);
    obj1.reset();
    BOOST_CHECK_EQUAL(obj1.refCount(),0);
}

BOOST_FIXTURE_TEST_CASE(CastNewDelete,MultiThreadFixture)
{
    PlainTestStruct* ptr=new PlainTestStructManaged(2000);
    BOOST_CHECK(ptr!=nullptr);
    delete ptr;

    SharedPtr<PlainTestStruct> obj1;
    SharedPtr<PlainTestStruct> obj2(new PlainTestStruct(11));
    SharedPtr<PlainTestStruct> obj3;
    obj3.reset(new PlainTestStruct(12));
    SharedPtr<PlainTestStructManaged> obj7;

    SharedPtr<PlainTestStructManaged> obj10(new PlainTestStructManaged(1000));
    BOOST_CHECK_EQUAL(obj10.refCount(),1);
    SharedPtr<PlainTestStructManaged> obj11=obj10;
    BOOST_CHECK_EQUAL(obj10.refCount(),2);
    BOOST_CHECK_EQUAL(obj11.refCount(),2);
    obj10.reset();
    obj11.reset();

    SharedPtr<PlainTestStruct> obj12(new PlainTestStructManaged(1000));
    BOOST_CHECK_EQUAL(obj12.refCount(),1);
    obj12.reset();

    SharedPtr<PlainTestStructManaged> obj15(new PlainTestStructManaged(1500));
    BOOST_CHECK_EQUAL(obj15.refCount(),1);
    auto obj16=obj15.staticCast<PlainTestStruct>();
    BOOST_CHECK_EQUAL(obj16.refCount(),2);
    obj15.reset();
    obj16.reset();

    BOOST_CHECK_EQUAL(obj1.refCount(),0);
    {
        SharedPtr<PlainTestStructManaged> obj4(new PlainTestStructManaged(15));
        SharedPtr<PlainTestStruct> obj5(new PlainTestStructManaged(16));
        BOOST_CHECK_EQUAL(obj4.refCount(),1);
        BOOST_CHECK_EQUAL(obj4->m_id,15);
        BOOST_CHECK_EQUAL(obj5.refCount(),1);
        BOOST_CHECK_EQUAL(obj5->m_id,16);
        obj1=obj4.template staticCast<PlainTestStruct>();
        BOOST_CHECK_EQUAL(obj1.refCount(),2);
        BOOST_CHECK_EQUAL(obj4.refCount(),2);
        BOOST_CHECK_EQUAL(obj1->m_id,15);
        BOOST_CHECK_EQUAL(obj4->m_id,15);

        SharedPtr<PlainTestStructManaged> obj8(new PlainTestStructManaged(20));
        obj7=obj8;
        BOOST_CHECK_EQUAL(obj7.refCount(),2);
        BOOST_CHECK_EQUAL(obj7->m_id,20);
        BOOST_CHECK_EQUAL(obj8.refCount(),2);
        BOOST_CHECK_EQUAL(obj8->m_id,20);
    }
    BOOST_CHECK_EQUAL(obj1.refCount(),1);
    BOOST_CHECK_EQUAL(obj1->m_id,15);
    obj1.reset();
    BOOST_CHECK_EQUAL(obj1.refCount(),0);
    BOOST_CHECK_EQUAL(obj2.refCount(),1);
    obj2.reset();
    BOOST_CHECK_EQUAL(obj2.refCount(),0);
    BOOST_CHECK_EQUAL(obj3.refCount(),1);
    obj3.reset();
    BOOST_CHECK_EQUAL(obj3.refCount(),0);
    BOOST_CHECK_EQUAL(obj7.refCount(),1);
    obj7.reset();
    BOOST_CHECK_EQUAL(obj7.refCount(),0);
}

BOOST_FIXTURE_TEST_CASE(Move,MultiThreadFixture)
{
    pmr::polymorphic_allocator<TestStruct> allocator1(HATN_COMMON_NAMESPACE::pmr::get_default_resource());
    auto p1=pointers_mempool::Pointers::allocateShared(allocator1,123);
    BOOST_CHECK_EQUAL(p1.refCount(),1);
    auto p2=std::move(p1);
    BOOST_CHECK_EQUAL(p2.refCount(),1);
    BOOST_CHECK_EQUAL(p2->m_id,123);
    // codechecker_intentional [all]
    BOOST_CHECK_EQUAL(p1.refCount(),0);
    pointers_mempool::Pointers::SharedPtr<TestStruct> p2_1(std::move(p2));
    BOOST_CHECK_EQUAL(p2_1.refCount(),1);
    BOOST_CHECK_EQUAL(p2_1->m_id,123);
    // codechecker_intentional [all]
    BOOST_CHECK_EQUAL(p2.refCount(),0);

    pmr::polymorphic_allocator<PlainTestStruct> allocator2(HATN_COMMON_NAMESPACE::pmr::get_default_resource());
    auto p3=pointers_mempool::Pointers::allocateShared(allocator2,123);
    BOOST_CHECK_EQUAL(p3.refCount(),1);
    auto p4=std::move(p3);
    BOOST_CHECK_EQUAL(p4.refCount(),1);
    BOOST_CHECK_EQUAL(p4->m_id,123);
    // codechecker_intentional [all]
    BOOST_CHECK_EQUAL(p3.refCount(),0);
    pointers_mempool::Pointers::SharedPtr<PlainTestStruct> p3_1(std::move(p4));
    BOOST_CHECK_EQUAL(p3_1.refCount(),1);
    BOOST_CHECK_EQUAL(p3_1->m_id,123);
    // codechecker_intentional [all]
    BOOST_CHECK_EQUAL(p4.refCount(),0);

    auto p5=pointers_std::Pointers::allocateShared(allocator1,123);
    BOOST_CHECK_EQUAL(p5.refCount(),1);
    auto p6=std::move(p5);
    BOOST_CHECK_EQUAL(p6.refCount(),1);
    BOOST_CHECK_EQUAL(p6->m_id,123);
    // codechecker_intentional [all]
    BOOST_CHECK_EQUAL(p5.refCount(),0);
    pointers_std::Pointers::SharedPtr<TestStruct> p5_1(std::move(p6));
    BOOST_CHECK_EQUAL(p5_1.refCount(),1);
    BOOST_CHECK_EQUAL(p5_1->m_id,123);
    // codechecker_intentional [all]
    BOOST_CHECK_EQUAL(p6.refCount(),0);

    auto p7=pointers_std::Pointers::allocateShared(allocator2,123);
    BOOST_CHECK_EQUAL(p7.refCount(),1);
    auto p8=std::move(p7);
    BOOST_CHECK_EQUAL(p8.refCount(),1);
    BOOST_CHECK_EQUAL(p8->m_id,123);
    // codechecker_intentional [all]
    BOOST_CHECK_EQUAL(p7.refCount(),0);
    pointers_std::Pointers::SharedPtr<PlainTestStruct> p7_1(std::move(p8));
    BOOST_CHECK_EQUAL(p7_1.refCount(),1);
    BOOST_CHECK_EQUAL(p7_1->m_id,123);
    // codechecker_intentional [all]
    BOOST_CHECK_EQUAL(p8.refCount(),0);
}

BOOST_FIXTURE_TEST_CASE(CheckUniquePtr,MultiThreadFixture)
{
    pmr::polymorphic_allocator<TestStruct> allocator(HATN_COMMON_NAMESPACE::pmr::get_default_resource());

    for (size_t i=0;i<2;i++)
    {
        pmr::UniquePtr<TestStruct> obj1;
        BOOST_CHECK(!obj1);
        BOOST_CHECK(obj1.isNull());
        BOOST_CHECK(obj1.get()==nullptr);

        auto obj2_make=pmr::makeUnique<TestStruct>(123,"Hello from hatn");
        auto obj2_allocate=pmr::makeUnique<TestStruct>(123,"Hello from hatn");

        auto& obj2=(i==0)?obj2_make:obj2_allocate;
        BOOST_CHECK(obj2);
        BOOST_CHECK(!obj2.isNull());
        BOOST_CHECK(obj2.get()!=nullptr);
        BOOST_CHECK_EQUAL(obj2->m_id,123);
        BOOST_CHECK_EQUAL(obj2->m_str,std::string("Hello from hatn"));
        BOOST_CHECK_EQUAL((*obj2).m_id,123);
        BOOST_CHECK_EQUAL((*obj2).m_str,std::string("Hello from hatn"));

        obj2->m_id=321;
        (*obj2).m_str="From hatn hello";
        BOOST_CHECK_EQUAL(obj2->m_id,321);
        BOOST_CHECK_EQUAL(obj2->m_str,std::string("From hatn hello"));

        pmr::UniquePtr<TestStruct> obj3(std::move(obj2));
        BOOST_CHECK(obj3);
        BOOST_CHECK(!obj3.isNull());
        BOOST_CHECK(obj3.get()!=nullptr);
        BOOST_CHECK_EQUAL(obj3->m_id,321);
        BOOST_CHECK_EQUAL(obj3->m_str,std::string("From hatn hello"));
        // codechecker_intentional [all]
        BOOST_CHECK(!obj2);
        // codechecker_intentional [all]
        BOOST_CHECK(obj2.isNull());
        // codechecker_intentional [all]
        BOOST_CHECK(obj2.get()==nullptr);

        obj1=std::move(obj3);
        BOOST_CHECK(obj1);
        BOOST_CHECK(!obj1.isNull());
        BOOST_CHECK(obj1.get()!=nullptr);
        BOOST_CHECK_EQUAL(obj1->m_id,321);
        BOOST_CHECK_EQUAL(obj1->m_str,std::string("From hatn hello"));
        // codechecker_intentional [all]
        BOOST_CHECK(!obj3);
        // codechecker_intentional [all]
        BOOST_CHECK(obj3.isNull());
        // codechecker_intentional [all]
        BOOST_CHECK(obj3.get()==nullptr);

        obj1.reset();
        BOOST_CHECK(!obj1);
        BOOST_CHECK(obj1.isNull());
        BOOST_CHECK(obj1.get()==nullptr);
    }

    auto obj4Alloc=pmr::allocateConstruct<TestStruct>(allocator,567,"Hello again");
    pmr::UniquePtr<TestStruct> obj4;
    obj4.reset(obj4Alloc,&allocator);
    BOOST_CHECK(obj4);
    BOOST_CHECK(!obj4.isNull());
    BOOST_CHECK(obj4.get()!=nullptr);
    BOOST_CHECK_EQUAL(obj4->m_id,567);
    BOOST_CHECK_EQUAL(obj4->m_str,std::string("Hello again"));

    auto obj5raw=new TestStruct(890,"Hello again and again");
    pmr::UniquePtr<TestStruct> obj5;
    obj5.reset(obj5raw);
    BOOST_CHECK(obj5);
    BOOST_CHECK(!obj5.isNull());
    BOOST_CHECK(obj5.get()!=nullptr);
    BOOST_CHECK_EQUAL(obj5->m_id,890);
    BOOST_CHECK_EQUAL(obj5->m_str,std::string("Hello again and again"));
}

BOOST_AUTO_TEST_SUITE_END()
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
