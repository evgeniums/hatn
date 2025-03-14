PROJECT(hatnmq)

SET (HEADERS
    include/hatn/mq/mq.h
    include/hatn/mq/mqerror.h
    include/hatn/mq/mqerrorcodes.h
    include/hatn/mq/scheduler.h
    include/hatn/mq/backgroundworker.h
    include/hatn/mq/message.h
    include/hatn/mq/producerclient.h
    include/hatn/mq/producerservice.h
    include/hatn/mq/consumerservice.h
    include/hatn/mq/consumerclient.h
    include/hatn/mq/notifier.h
    include/hatn/mq/listener.h
    include/hatn/mq/syncservice.h
    include/hatn/mq/syncclient.h
    include/hatn/mq/proxynotifier.h
    include/hatn/mq/proxylistener.h
)

SET (HEADERS
    ${HEADERS}
    include/hatn/mq/ipp/producerservice.ipp
)

SET (SOURCES
        src/mqerror.cpp
)

BUILD_HATN_MODULE()
