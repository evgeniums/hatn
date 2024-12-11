/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/x509certificatechain.cpp
 * 	Wrapper/container of X.509 certificate chain
 */
/****************************************************************************/

#include <hatn/common/runonscopeexit.h>

#include <hatn/crypt/x509certificate.h>
#include <hatn/crypt/x509certificatechain.h>

namespace hatn {

using namespace common;

namespace crypt {

/*********************** X509CertificateChain ***********************/

//---------------------------------------------------------------
common::Error X509CertificateChain::loadCerificates(const std::vector<common::SharedPtr<X509Certificate> > &certs)
{
    reset();
    bool success=true;
    common::RunOnScopeExit guard(
        [this,&success]()
        {
            if (!success)
            {
                reset();
            }
        }
    );
    for (auto&& it:certs)
    {
        HATN_CHECK_RETURN(addCertificate(*it))
    }
    return common::Error();
}

//---------------------------------------------------------------
HATN_CRYPT_NAMESPACE_END
