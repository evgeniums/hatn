/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/withpassphrase.h
  */

/****************************************************************************/

#ifndef HATNWITHPASSPHRASE_H
#define HATNWITHPASSPHRASE_H

#include <hatn/dataunit/syntax.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT(with_passphrase,
    HDU_FIELD(passphrase,TYPE_STRING,90)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNWITHPASSPHRASE_H
