/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file common/streamchaintraits.h
  *
  *
  */

/****************************************************************************/

#ifndef HATNSTREAMCHAINTRAITS_H
#define HATNSTREAMCHAINTRAITS_H

#include <functional>

#include <boost/hana.hpp>

#include <hatn/validator/utils/foreach_if.hpp>

#include <hatn/common/meta/errorpredicate.h>
#include <hatn/common/meta/tupletypec.h>
#include <hatn/common/meta/replicatetotuple.h>
#include <hatn/common/spanbuffer.h>
#include <hatn/common/stream.h>

HATN_COMMON_NAMESPACE_BEGIN

template <typename ... Streams>
class StreamChainTraits
{
    public:

        constexpr static const size_t StreamsCount=sizeof...(Streams);
        static_assert(StreamsCount>0,"At least one stream must be added to the chain");

        StreamChainTraits() : m_streams(common::replicateToTuple(nullptr,boost::hana::size_c<StreamsCount>))
        {}

        /**
         * @brief Constructor
         * @param streams Plain pointers to streams in the chain
         */
        template <typename ...Ts>
        StreamChainTraits(
            Ts... streams
        ) : m_streams(streams...)
        {
            initStreams();
        }

        template <typename ...Ts>
        void setStreams(Ts... streams)
        {
            m_streams=hana::make_tuple(streams...);
            initStreams();
        }

        //! Write to stream
        void write(
            const char* data,
            std::size_t size,
            std::function<void (const Error&,size_t)> callback
            )
        {
            front()->write(data,size,std::move(callback));
        }

        void write(
            SpanBuffers buffers,
            std::function<void (const Error&,size_t,SpanBuffers)> callback
            )
        {
            front()->write(std::move(buffers),std::move(callback));
        }

        //! Read from stream
        void read(
            char* data,
            std::size_t maxSize,
            std::function<void (const Error&,size_t)> callback
            )
        {
            front()->read(data,maxSize,std::move(callback));
        }

        inline void readAll(
            char* data,
            std::size_t expectedSize,
            std::function<void (const Error&,size_t)> callback
            )
        {
            front()->readAll(data,expectedSize,std::move(callback));
        }

        void cancel()
        {
            hana::for_each(m_streams,[](auto&& stream){
                stream->cancel();
            });
        }

        void prepare(
            std::function<void (const Error &)> callback
        )
        {
            auto self=this;
            hana::eval_if(
                hana::equal(hana::size(m_streams),hana::size_c<1>),
                [&](auto _)
                {
                    _(self)->front()->prepare(_(callback));
                },
                [&](auto _)
                {
                    auto nextStreams=hana::drop_front(_(self)->m_streams,hana::size_c<1>);

                    auto each=[](auto&& state,auto&& stream)
                    {
                        auto cb=[state{std::move(state)}](const Error & ec)
                        {
                            auto prevCb=hana::second(state);
                            if (ec)
                            {
                                return prevCb(ec);
                            }
                            auto prevStream=hana::first(state);
                            prevStream->prepare(prevCb);
                        };
                        return hana::make_pair(stream,std::move(cb));
                    };

                    auto st=hana::fold(nextStreams,hana::make_pair(_(self)->front(),_(callback)),each);
                    auto lastStream=hana::first(st);
                    auto lastCb=hana::second(st);
                    lastStream->prepare(lastCb);
                }
            );
        }

        void close(const std::function<void (const Error &)>& callback, bool destroying)
        {
            std::ignore=destroying;
            auto self=this;
            hana::eval_if(
                hana::equal(hana::size(m_streams),hana::size_c<1>),
                [&](auto _)
                {
                    _(self)->front()->close(_(callback));
                },
                [&](auto _)
                {
                    auto nextStreams=hana::drop_back(_(self)->m_streams,hana::size_c<1>);

                    auto each=[](auto&& state,auto&& stream)
                    {
                        auto cb=[state{std::move(state)}](const Error & ec)
                        {
                            auto nextCb=hana::second(state);
                            //! @todo Collect errors
                            std::ignore=ec;
                            auto nextStream=hana::first(state);
                            nextStream->close(nextCb);
                        };
                        return hana::make_pair(stream,std::move(cb));
                    };

                    auto lastStream=hana::back(_(self)->m_streams);
                    auto st=hana::fold_right(nextStreams,hana::make_pair(lastStream,_(callback)),each);

                    auto firstStream=hana::first(st);
                    auto firstCb=hana::second(st);
                    firstStream->close(firstCb);
                }
            );
        }

    private:

        auto front() noexcept -> decltype(auto)
        {
            return hana::front(m_streams);
        }

        constexpr static auto toPointerTuple()
        {
            return hana::transform(hana::tuple_t<Streams...>,
                                   [](auto x)
                                   {
                                       return hana::type_c<typename decltype(x)::type*>;
                                   }
                                   );
        }

        void initStreams()
        {
            auto self=this;
            hana::eval_if(
                hana::equal(hana::size(m_streams),hana::size_c<1>),
                [](auto)
                {},
                [&](auto _)
                {
                    auto chainStreams=hana::drop_back(_(self)->m_streams,hana::size_c<1>);

                    auto each=[chainStreams](auto&& stream, auto&& idx)
                    {
                        auto nextIdx=hana::plus(idx,hana::size_c<1>);
                        auto nextStream=hana::at(chainStreams,nextIdx);

                        auto writeNext=[nextStream](
                                             const char* data,
                                             std::size_t size,
                                             std::function<void (const Error&,size_t)> callback
                                             )
                        {
                            nextStream->write(data,size,std::move(callback));
                        };

                        auto readNext=[nextStream](
                                            char* data,
                                            std::size_t maxSize,
                                            std::function<void (const Error&,size_t)> callback
                                            )
                        {
                            nextStream->read(data,maxSize,std::move(callback));
                        };

                        stream->setWriteNext(std::move(writeNext));
                        stream->setReadNext(std::move(readNext));

                        return true;
                    };

                    validator::foreach_if(chainStreams,true_predicate,each);
                }
                );
        }

        using StreamsTs=tupleCToTupleType<decltype(toPointerTuple())>;
        StreamsTs m_streams;
};

template <typename ... Streams>
using StreamChainGatherTraits=StreamGatherTraits<StreamChainTraits<Streams...>>;

template <typename ... Streams>
class StreamChainGather : public Stream<StreamChainGatherTraits<Streams...>>
{
    public:

        using Stream<StreamChainGatherTraits<Streams...>>::Stream;

        template <typename ...Ts>
        void setStreams(Ts... streams)
        {
            this->traits().setStreams(streams...);
        }
};

HATN_COMMON_NAMESPACE_END

#endif // HATNSTREAMCHAIN_H
