#include <boost/hana/ext/std/tuple.hpp>

#include <boost/test/unit_test.hpp>
namespace tt = boost::test_tools;

#include <hatn/common/taskcontext.h>

HATN_USING
HATN_COMMON_USING

namespace {

// class TaskSubcontextT
// {
// public:

//     /**
//          * @brief Constructor.
//          * @param taskContext Main task context.
//          */
//     TaskSubcontextT(TaskContext* mainContext=nullptr) : m_mainCtx(mainContext)
//     {}

//     /**
//          * @brief Get main task context.
//          * @return Result.
//          */
//     const TaskContext& mainCtx() const noexcept
//     {
//         return *m_mainCtx;
//     }

//     /**
//          * @brief Get main task context.
//          * @return Result.
//          */
//     TaskContext& mainCtx() noexcept
//     {
//         return *m_mainCtx;
//     }

//     void setMainCtx(TaskContext* mainContext)
//     {
//         m_mainCtx=mainContext;
//     }

// private:

//     TaskContext* m_mainCtx;
// };

// template <typename T>
// class TaskSubcontextTT : public TaskSubcontextT, public T
// {
// public:

//     template <typename ...Args>
//     TaskSubcontextTT(
//             TaskContext* mainContext,
//             Args&& ...args
//         ) : TaskSubcontextT(mainContext),
//             T(std::forward<Args>(args)...)
//     {}

//     template <typename ...Args>
//     TaskSubcontextTT(
//             Args&& ...args
//         ) : T(std::forward<Args>(args)...)
//     {}

//     template <typename Ts, std::size_t... I>
//     TaskSubcontextTT(
//             Ts&& ts,
//             std::index_sequence<I...>
//         ) : TaskSubcontextTT(std::get<I>(std::forward<Ts>(ts))...)
//     {}

//     template <template <typename ...> class Ts, typename ...Types>
//     TaskSubcontextTT(
//             Ts<Types...>&& ts
//         ) : TaskSubcontextTT(
//                 std::forward<Ts<Types...>>(ts),
//                 std::make_index_sequence<std::tuple_size<std::remove_reference_t<Ts<Types...>>>::value>{}
//             )
//     {}
// };

template <typename Subcontexts>
struct SubcontextRefsT
{
    constexpr static auto toRefs()
    {
        tupleToTupleCType<Subcontexts> tc;
        auto rtc=hana::transform(
            tc,
            [](auto&& v)
            {
                using type=typename std::decay_t<decltype(v)>::type;
                return hana::type_c<std::reference_wrapper<const type>>;
            }
            );
        return hana::unpack(rtc,hana::template_<hana::tuple>);
    }

    using typeC=decltype(toRefs());
    using type=typename typeC::type;
};

template <typename T>
auto subctxRefsT(const T& subcontexts)
{
    return hana::fold(
        subcontexts,
        hana::make_tuple(),
        [](auto&& ts, const auto& subctx)
        {
            return hana::append(ts,std::cref(subctx));
        }
    );
}

template <typename Subcontexts, typename BaseTaskContext=TaskContext>
class ActualTaskContextT : public BaseTaskContext
{
public:

    using selfT=ActualTaskContext<Subcontexts,BaseTaskContext>;

    template <typename BaseTs, typename Tts>
    ActualTaskContextT(BaseTs&& baseTs, Tts&& tts):
        BaseTaskContext(std::forward<BaseTs>(baseTs)),
        m_subcontexts(std::forward<Tts>(tts))
        ,
        refs(subctxRefsT(m_subcontexts))
    {
        hana::for_each(
            m_subcontexts,
            [this](auto&& subCtx)
            {
                subCtx.setMainCtx(this);
            }
        );
    }

    template <typename Tts>
    ActualTaskContextT(Tts&& tts)
        : m_subcontexts(std::forward<Tts>(tts))
        ,
          refs(subctxRefsT(m_subcontexts))
    {
        hana::for_each(
            m_subcontexts,
            [this](auto&& subCtx)
            {
                subCtx.setMainCtx(this);
            }
        );
    }

    /**
         * @brief Get interface of base class.
         * @return Result.
         */
    BaseTaskContext* baseTaskContext() noexcept
    {
        return static_cast<BaseTaskContext*>(this);
    }

    /**
         * @brief Get tuple of subcontexts.
         * @return Const reference to subcontexts.
         */
    const Subcontexts& subcontexts() const noexcept
    {
        return m_subcontexts;
    }

    /**
         * @brief Get tuple of subcontexts.
         * @return Reference to subcontexts.
         */
    Subcontexts& subcontexts() noexcept
    {
        return m_subcontexts;
    }

    Subcontexts m_subcontexts;
    decltype(subctxRefsT(m_subcontexts)) refs;
};

class A0
{
    public:

        A0(const A0&)
        {
            std::cout<<"A0 copy ctor"<<std::endl;
        }

        A0(A0&&)
        {
            std::cout<<"A0 move ctor"<<std::endl;
        }

        A0()
        {
            std::cout<<"A0 default ctor"<<std::endl;
        }
};

class A00
{
public:

    A00(const A00&)
    {
        std::cout<<"A00 copy ctor"<<std::endl;
    }

    A00(A00&&)
    {
        std::cout<<"A00 move ctor"<<std::endl;
    }

    A00()
    {
        std::cout<<"A00 default ctor"<<std::endl;
    }
};

class A01
{
public:

    A01(const A01&)
    {
        std::cout<<"A01 copy ctor"<<std::endl;
    }

    A01(A01&&)
    {
        std::cout<<"A01 move ctor"<<std::endl;
    }

    A01()
    {
        std::cout<<"A01 default ctor"<<std::endl;
    }
};

class A1
{
    public:

        A1(uint32_t var1,
           std::string var2,
           double var3,
           A0& var4,
           A00 var5,
           A01&& var6
        ) : var1(var1),
            var2(std::move(var2)),
            var3(var3),
            var4(var4),
            var5(std::move(var5)),
            var6(std::move(var6))
        {}

        uint32_t var1;
        std::string var2;
        double var3;
        A0& var4;
        A00 var5;
        A01 var6;
};

class A2
{
public:

    A2(uint32_t var1,
       std::string var2,
       double var3,
       A0& var4,
       A00 var5,
       A01&& var6
       ) : var1(var1),
        var2(std::move(var2)),
        var3(var3),
        var4(var4),
        var5(std::move(var5)),
        var6(std::move(var6))
    {}

    uint32_t var1;
    std::string var2;
    double var3;
    A0& var4;
    A00 var5;
    A01 var6;
};

class A3
{
};

using A1Ctx=TaskSubcontextT<A1>;
using A2Ctx=TaskSubcontextT<A2>;

template <typename ...Types>
struct makeTaskContextT1
{
    template <typename BaseArgs, typename SubcontextsArgs>
    auto operator()(BaseArgs&& baseArgs, SubcontextsArgs&& subcontextsArgs) const
    {
        using type=typename common::detail::ActualTaskContexTraits<Types...>::type;
        return makeShared<type>(std::forward<BaseArgs>(baseArgs),std::forward<SubcontextsArgs>(subcontextsArgs));
    }

    template <typename SubcontextsArgs>
    auto operator()(SubcontextsArgs&& subcontextsArgs) const
    {
        using type=typename common::detail::ActualTaskContexTraits<Types...>::type;
        return makeShared<type>(std::forward<SubcontextsArgs>(subcontextsArgs));
    }
};
template <typename ...Types>
constexpr makeTaskContextT1<Types...> makeTaskContext1{};

struct B1
{
    B1(uint32_t val=0):val1(val)
    {}

    uint32_t val1;
};
using B1Ctx=common::TaskContextType<B1>;

struct B2
{
    B2(std::string val=std::string()):val1(val)
    {}

    std::string val1;
};

}

HATN_TASK_CONTEXT_DECLARE(A1)
HATN_TASK_CONTEXT_DEFINE(A1)

HATN_TASK_CONTEXT_DECLARE(A2)
HATN_TASK_CONTEXT_DEFINE(A2)

HATN_TASK_CONTEXT_DECLARE(A3,HATN_NO_EXPORT)
HATN_TASK_CONTEXT_DEFINE(A3)

HATN_TASK_CONTEXT_DECLARE(B1)
HATN_TASK_CONTEXT_DEFINE(B1)

HATN_TASK_CONTEXT_DECLARE(B2)
HATN_TASK_CONTEXT_DEFINE(B2)

BOOST_AUTO_TEST_SUITE(TestTaskContext)

BOOST_AUTO_TEST_CASE(MakeSubcontext)
{
    TaskContext mainCtx;

    std::cout << "Create a1" << std::endl;
    A0 a0;
    auto a1=A1Ctx{std::forward_as_tuple(&mainCtx,uint32_t(100),"Hello!",100.20,a0,A00{},A01{})};

    BOOST_CHECK_EQUAL(a1.var1,100);
    BOOST_CHECK_EQUAL(a1.var2,std::string("Hello!"));
    BOOST_TEST(a1.var3==100.20,tt::tolerance(0.001));

#if __cplusplus >= 201703L
    std::cout << "Create a2" << std::endl;
    auto a2=std::make_from_tuple<A1Ctx>(std::forward_as_tuple(&mainCtx,uint32_t(100),"Hello!",100.20,a0,A00{},A01{}));
    BOOST_CHECK_EQUAL(a2.var1,100);
    BOOST_CHECK_EQUAL(a2.var2,std::string("Hello!"));
    BOOST_TEST(a2.var3==100.20,tt::tolerance(0.001));
#endif
}

BOOST_AUTO_TEST_CASE(MakeContext)
{
    TaskContext mainCtx1;
    auto tts0=std::forward_as_tuple(
            std::forward_as_tuple(A00{})
        );
    auto ts0=hana::transform(
        tts0,
        [&mainCtx1](auto&& ts)
        {
            return hana::insert(std::forward<decltype(ts)>(ts),hana::size_c<0>,&mainCtx1);
        }
    );
    std::ignore=ts0;

    A0 a0;

    std::cout << "Create std::tuple<A1Ctx>" << std::endl;
    hana::tuple<A1Ctx> a1Ctx{
        std::forward_as_tuple(
            std::forward_as_tuple(uint32_t(100),"Hello!",100.20,a0,A00{},A01{})
        )};

    std::cout << "Create ctx1" << std::endl;
    ActualTaskContext<std::tuple<A1Ctx>> ctx1{
        std::forward_as_tuple(
            std::forward_as_tuple(uint32_t(100),"Hello!",100.20,a0,A00{},A01{})
        ),
        "ctx1"
    };

    auto& subctx1=ctx1.get<A1>();
    BOOST_CHECK_EQUAL(subctx1.var1,100);
    BOOST_CHECK_EQUAL(subctx1.var2,std::string("Hello!"));
    BOOST_TEST(subctx1.var3==100.20,tt::tolerance(0.001));
    BOOST_CHECK_EQUAL(std::string(ctx1.id()),std::string("ctx1"));

    std::cout << "Create ctx2" << std::endl;
    auto ctx2=makeTaskContext1<A1>(
                    subcontexts(
                        subcontext(uint32_t(100),"Hello!",100.20,a0,A00{},A01{})
                    ),
                    "ctx2"
                );

    std::cout << "Create ctx3_" << std::endl;
    ActualTaskContext<std::tuple<A1Ctx,A2Ctx>> ctx3_{
        std::forward_as_tuple(
                std::forward_as_tuple(uint32_t(100),"Hello!",100.20,a0,A00{},A01{}),
                std::forward_as_tuple(uint32_t(200),"Hi!",200.10,a0,A00{},A01{})
            ),
        "ctx3_"
    };
    std::ignore=ctx3_;

    std::cout << "Create ctx3" << std::endl;
    auto ctx3=makeTaskContext<A1,A2,A3>(
                subcontexts(
                    subcontext(uint32_t(100),"Hello!",100.20,a0,A00{},A01{}),
                    subcontext(uint32_t(200),"Hi!",200.10,a0,A00{},A01{}),
                    subcontext()
                ),
                "ctx3"
            );
    BOOST_CHECK_EQUAL(std::string(ctx3->id()),std::string("ctx3"));
    auto& subctx3_1=ctx3->get<A1>();
    BOOST_CHECK_EQUAL(subctx3_1.var1,100);
    BOOST_CHECK_EQUAL(subctx3_1.var2,std::string("Hello!"));
    BOOST_TEST(subctx3_1.var3==100.20,tt::tolerance(0.001));
    auto& subctx3_2=ctx3->get<A2>();
    BOOST_CHECK_EQUAL(subctx3_2.var1,200);
    BOOST_CHECK_EQUAL(subctx3_2.var2,std::string("Hi!"));
    BOOST_TEST(subctx3_2.var3==200.10,tt::tolerance(0.001));

    std::cout << "Create ctx4" << std::endl;
    auto ctx4=makeTaskContext<A1,A2,A3>(
        subcontexts(
            subcontext(uint32_t(100),"Hello!",100.20,a0,A00{},A01{}),
            subcontext(uint32_t(200),"Hi!",200.10,a0,A00{},A01{}),
            subcontext()
            )
        );
    std::cout << "ctx4->id()=" << ctx4->id() << std::endl;
    ctx4->setId("ctx4");
    BOOST_CHECK_EQUAL(std::string(ctx4->id()),std::string("ctx4"));
    auto& subctx4_1=ctx4->get<A1>();
    BOOST_CHECK_EQUAL(subctx4_1.var1,100);
    BOOST_CHECK_EQUAL(subctx4_1.var2,std::string("Hello!"));
    BOOST_TEST(subctx4_1.var3==100.20,tt::tolerance(0.001));
    auto& subctx4_2=ctx4->get<A2>();
    BOOST_CHECK_EQUAL(subctx4_2.var1,200);
    BOOST_CHECK_EQUAL(subctx4_2.var2,std::string("Hi!"));
    BOOST_TEST(subctx4_2.var3==200.10,tt::tolerance(0.001));

    std::cout << "Create ctx5" << std::endl;
    using ctx5Type=typename allocateTaskContextT<A1,A2,A3>::type;
    pmr::polymorphic_allocator<ctx5Type> alloc5;
    auto ctx5=allocateTaskContext<A1,A2,A3>(
        alloc5,
        subcontexts(
            subcontext(uint32_t(100),"Hello!",100.20,a0,A00{},A01{}),
            subcontext(uint32_t(200),"Hi!",200.10,a0,A00{},A01{}),
            subcontext()
            ),
        "ctx5"
        );
    BOOST_CHECK_EQUAL(std::string(ctx5->id()),std::string("ctx5"));
    auto& subctx5_1=ctx5->get<A1>();
    BOOST_CHECK_EQUAL(subctx5_1.var1,100);
    BOOST_CHECK_EQUAL(subctx5_1.var2,std::string("Hello!"));
    BOOST_TEST(subctx5_1.var3==100.20,tt::tolerance(0.001));
    auto& subctx5_2=ctx5->get<A2>();
    BOOST_CHECK_EQUAL(subctx5_2.var1,200);
    BOOST_CHECK_EQUAL(subctx5_2.var2,std::string("Hi!"));
    BOOST_TEST(subctx5_2.var3==200.10,tt::tolerance(0.001));

    std::cout << "Create ctx6" << std::endl;
    auto ctx6=allocateTaskContext<A1,A2,A3>(
        alloc5,
        subcontexts(
            subcontext(uint32_t(100),"Hello!",100.20,a0,A00{},A01{}),
            subcontext(uint32_t(200),"Hi!",200.10,a0,A00{},A01{}),
            subcontext()
            )
        );
    std::cout << "ctx6->id()=" << ctx6->id() << std::endl;
    ctx6->setId("ctx6");
    BOOST_CHECK_EQUAL(std::string(ctx6->id()),std::string("ctx6"));
    auto& subctx6_1=ctx6->get<A1>();
    BOOST_CHECK_EQUAL(subctx6_1.var1,100);
    BOOST_CHECK_EQUAL(subctx6_1.var2,std::string("Hello!"));
    BOOST_TEST(subctx6_1.var3==100.20,tt::tolerance(0.001));
    auto& subctx6_2=ctx6->get<A2>();
    BOOST_CHECK_EQUAL(subctx6_2.var1,200);
    BOOST_CHECK_EQUAL(subctx6_2.var2,std::string("Hi!"));
    BOOST_TEST(subctx6_2.var3==200.10,tt::tolerance(0.001));
}

BOOST_AUTO_TEST_CASE(NestedContext)
{
    auto xs=hana::tuple_t<B2,B1Ctx>;
    auto last=hana::back(xs);
    using lastT=typename decltype(last)::type;
    static_assert(hana::is_a<TaskContexTag,lastT>,"");
    auto tc=hana::make_pair(hana::drop_back(xs),last);
    auto base=hana::second(tc);

    auto wrappersC=hana::transform(
        hana::first(tc),
        [](auto&& v)
        {
            using type=typename std::decay_t<decltype(v)>::type;
            return hana::eval_if(
                hana::is_a<TaskSubcontextTag,type>,
                [&](auto&&)
                {
                    return hana::type_c<type>;
                },
                [&](auto&&)
                {
                    return hana::type_c<TaskSubcontextT<type>>;
                }
                );
        }
        );
    using wrappersT=common::tupleCToStdTupleType<std::decay_t<decltype(wrappersC)>>;
    auto tmpl=hana::template_<ActualTaskContext>(hana::type_c<wrappersT>,base);
    using type0=typename decltype(tmpl)::type;
    type0 tt0;
    type0 tt1{
        subcontexts(
            subcontext("Hello!")
        )
    };
    B1Ctx b1{
        subcontexts(
            subcontext(100)
        ),
        basecontext("ctx3")
    };
    static_assert(std::is_base_of<B1Ctx,type0>::value,"");
    type0 tt2(
        subcontexts(
            subcontext("Hello!")
        ),
        subcontexts(
            subcontext(100)
        ),
        basecontext("ctx3")
    );

    using type1=TaskContextType<B2,B1Ctx>;
    type1 ctx0;
    type1 ctx1(
        subcontexts(
            subcontext("Hello!")
        ),
        subcontexts(
            subcontext(100)
        ),
        "ctx1"
    );
    type1 ctx2(
        subcontexts(
            subcontext("Hello!")
            ),
        subcontexts(
            subcontext(100)
            )
    );
    type1 ctx3(
        subcontexts(
            subcontext("Hello!")
            )
    );
    type1 ctx4;

    BOOST_CHECK_EQUAL(std::string(ctx1.id()),std::string("ctx1"));
    BOOST_CHECK_EQUAL(ctx1.get().val1,std::string("Hello!"));
    BOOST_CHECK_EQUAL(ctx1.get<B1>().val1,100);

    auto sctx1=makeTaskContext<B2,B1Ctx>(
        subcontexts(
            subcontext("Hello!")
            ),
        subcontexts(
            subcontext(100)
            ),
        "ctx1"
        );
    auto sctx2=makeTaskContext<B2,B1Ctx>(
        subcontexts(
            subcontext("Hello!")
            ),
        subcontexts(
            subcontext(100)
            )
        );
    auto sctx3=makeTaskContext<B2,B1Ctx>(
        subcontexts(
            subcontext("Hello!")
            )
        );
    auto sctx4=makeTaskContext<B2,B1Ctx>();
    auto sctx5=makeTaskContext<B2,B1Ctx>(
        subcontexts(
            subcontext("Hello!")
            ),
        subcontexts(
            subcontext(100)
            ),
        basecontext("ctx5",3)
    );
    auto sctx6=makeTaskContext<B2,B1Ctx>(
        subcontexts(
            subcontext("Hello!")
            ),
        subcontexts(
            subcontext(100)
            ),
        "ctx6",
        5
    );

    BOOST_CHECK_EQUAL(std::string(sctx1->id()),std::string("ctx1"));
    BOOST_CHECK_EQUAL(sctx1->get().val1,std::string("Hello!"));
    BOOST_CHECK_EQUAL(sctx1->get<B1>().val1,100);
    BOOST_CHECK_EQUAL(std::string(sctx5->id()),std::string("ctx5"));
    BOOST_CHECK_EQUAL(static_cast<int>(sctx5->tz()),static_cast<int>(3));
    BOOST_CHECK_EQUAL(std::string(sctx6->id()),std::string("ctx6"));
    BOOST_CHECK_EQUAL(static_cast<int>(sctx6->tz()),static_cast<int>(5));

    pmr::polymorphic_allocator<TaskContextType<B2,B1Ctx>> alloc;
    auto actx1=allocateTaskContext<B2,B1Ctx>(
        alloc,
        subcontexts(
            subcontext("Hello!")
            ),
        subcontexts(
            subcontext(100)
            ),
        "ctx1"
        );
    auto actx2=allocateTaskContext<B2,B1Ctx>(
        alloc,
        subcontexts(
            subcontext("Hello!")
            ),
        subcontexts(
            subcontext(100)
            )
        );
    auto actx3=allocateTaskContext<B2,B1Ctx>(
        alloc,
        subcontexts(
            subcontext("Hello!")
            )
        );
    auto actx4=allocateTaskContext<B2,B1Ctx>(alloc);

    BOOST_CHECK_EQUAL(std::string(actx1->id()),std::string("ctx1"));
    BOOST_CHECK_EQUAL(actx1->get().val1,std::string("Hello!"));
    BOOST_CHECK_EQUAL(actx1->get<B1>().val1,100);
    BOOST_CHECK_EQUAL(b1.get<B1>().val1,100);
}

BOOST_AUTO_TEST_SUITE_END()
