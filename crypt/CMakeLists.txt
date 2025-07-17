PROJECT(hatncrypt)

SET (HEADERS
    include/hatn/crypt/crypt.h
    include/hatn/crypt/crypterror.h
    include/hatn/crypt/x509certificate.h
    include/hatn/crypt/securestream.h
    include/hatn/crypt/securestreamtypes.h
    include/hatn/crypt/securestreamcontext.h
    include/hatn/crypt/cryptplugin.h
    include/hatn/crypt/securekey.h
    include/hatn/crypt/diffiehellman.h
    include/hatn/crypt/dh.h
    include/hatn/crypt/dhlegacy.h
    include/hatn/crypt/keycontainer.h
    include/hatn/crypt/sessionticketkey.h
    include/hatn/crypt/x509certificatechain.h
    include/hatn/crypt/publickey.h
    include/hatn/crypt/symmetriccipher.h
    include/hatn/crypt/cipherworker.h
    include/hatn/crypt/cipherworker.ipp
    include/hatn/crypt/symmetricworker.h
    include/hatn/crypt/symmetricworker.ipp
    include/hatn/crypt/digest.h
    include/hatn/crypt/hmac.h
    include/hatn/crypt/cryptalgorithm.h
    include/hatn/crypt/encrypthmac.h
    include/hatn/crypt/kdf.h
    include/hatn/crypt/pbkdf.h
    include/hatn/crypt/hkdf.h
    include/hatn/crypt/signature.h
    include/hatn/crypt/passphrasekey.h
    include/hatn/crypt/passwordgenerator.h
    include/hatn/crypt/randomgenerator.h
    include/hatn/crypt/ecdh.h
    include/hatn/crypt/aead.h
    include/hatn/crypt/aeadworker.h
    include/hatn/crypt/aeadworker.ipp
    include/hatn/crypt/mac.h
    include/hatn/crypt/encryptmac.h
    include/hatn/crypt/keyprotector.h
    include/hatn/crypt/cryptcontainerheader.h
    include/hatn/crypt/cryptcontainer.h
    include/hatn/crypt/cryptcontainer.ipp
    include/hatn/crypt/ciphersuite.h
    include/hatn/crypt/cryptfile.h
    include/hatn/crypt/ciphernonealgorithm.h
    include/hatn/crypt/x509certificatestore.h

    include/hatn/crypt/cryptdataunits.h

    include/hatn/crypt/asymmetricworker.h
    include/hatn/crypt/passphraseprotectedkey.h
    include/hatn/crypt/checksharedsecret.h
)

SET (SOURCES
    src/crypterror.cpp
    src/x509certificate.cpp
    src/securestream.cpp
    src/securestreamcontext.cpp
    src/cryptplugin.cpp
    src/securekey.cpp
    src/dh.cpp
    src/dhlegacy.cpp
    src/sessionticketkey.cpp
    src/x509certificatechain.cpp
    src/passwordgenerator.cpp
    src/mac.cpp
    src/digest.cpp
    src/encryptmac.cpp
    src/cryptalgorithm.cpp
    src/keyprotector.cpp
    src/cryptcontainer.cpp
    src/ciphersuite.cpp
    src/cryptfile.cpp

    src/cryptdataunits.cpp
    src/passphraseprotectedkey.cpp
)

BUILD_HATN_MODULE()
