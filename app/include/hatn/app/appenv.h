/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file app/appenv.h
  *
  */

/****************************************************************************/

#ifndef HATNAPPENV_H
#define HATNAPPENV_H

#include <hatn/common/env.h>
#include <hatn/common/translate.h>
#include <hatn/common/logger.h>

#include <hatn/logcontext/withlogger.h>

#include <hatn/crypt/ciphersuite.h>

#include <hatn/db/asyncclient.h>

#include <hatn/app/app.h>

HATN_APP_NAMESPACE_BEGIN

using Logger=HATN_LOGCONTEXT_NAMESPACE::WithLogger;
using LoggerHandlerBuilder=HATN_LOGCONTEXT_NAMESPACE::LoggerHandlerBuilder;
using Db=HATN_DB_NAMESPACE::AsyncDb;
using AllocatorFactory=common::pmr::WithFactory;
using CipherSuites=HATN_CRYPT_NAMESPACE::WithCipherSuites;
using Translator=common::WithTranslator;
using Threads=common::WithMappedThreads;

using AppEnv=common::Env<AllocatorFactory,Threads,Logger,Db,CipherSuites,Translator>;

HATN_APP_NAMESPACE_END

#endif // HATNAPPENV_H
