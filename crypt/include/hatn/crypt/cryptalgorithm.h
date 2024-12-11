/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/cryptalgotithm.h
  *
  *   Base classes for cryptigraphic algorithms and engines
  *
  */

/****************************************************************************/

#ifndef HATNCRYPTALG_H
#define HATNCRYPTALG_H

#include <hatn/common/error.h>
#include <hatn/common/fixedbytearray.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/crypterror.h>

HATN_CRYPT_NAMESPACE_BEGIN

class CryptPlugin;

//! Wrapper of backend crypt engine
class CryptEngine final
{
    public:

        /**
         * @brief Ctor
         * @param plugin Cryptographic plugin implementing backend
         * @param handler Native engine handler in the plugin
         */
        CryptEngine(CryptPlugin* plugin, void* handler=nullptr, const char* name=nullptr) noexcept
            : m_plugin(plugin), m_handler(handler), m_name(name)
        {}

        //! Dtor
        ~CryptEngine()
        {
            if (m_freeNative)
            {
                m_freeNative();
            }
        }

        CryptEngine(const CryptEngine&)=delete;
        CryptEngine(CryptEngine&&) =delete;
        CryptEngine& operator=(const CryptEngine&)=delete;
        CryptEngine& operator=(CryptEngine&&) =delete;

        //! Native handler of backend engine
        void* handler() const noexcept
        {
            return m_handler;
        }

        //! Cryptographic plugin implementing backend
        CryptPlugin* plugin() const noexcept
        {
            return m_plugin;
        }

        //! Reintepret casted native handler of backend engine
        template <typename T> T* nativeHandler() const noexcept
        {
            return reinterpret_cast<T*>(handler());
        }

        /**
         * @brief Set function to be called to destroy backend engine
         * @param fn Function to destroy backen engine
         *
         * @attention This function must not use any variables of derived class! Use free functions only.
         */
        void setFreeNativeFn(std::function<void ()> fn) noexcept
        {
            m_freeNative=std::move(fn);
        }

        const char* name() const noexcept
        {
            return m_name.c_str();
        }
        void setName(const char* name) noexcept
        {
            m_name.load(name);
        }

    private:

        CryptPlugin* m_plugin;
        void* m_handler;
        std::function<void ()> m_freeNative;
        common::FixedByteArray256 m_name;
};

//! Base class for all cryptographic classes created via cryptlugin builders
class CryptEngineAware
{
    public:

        /**
         * @brief Ctor
         * @param engine Wrapper of backend crypt engine
         */
        CryptEngineAware(const CryptEngine* engine):m_engine(engine)
        {}

        //! Get engine
        inline const CryptEngine* engine() const noexcept
        {
            return m_engine;
        }

    private:

        const CryptEngine* m_engine;
};

class SymmetricKey;
class PrivateKey;
class MACKey;
class SignatureSign;
class SignatureVerify;
class PublicKey;

//! Base class for cryptigraphic algorithms
class HATN_CRYPT_EXPORT CryptAlgorithm : public CryptEngineAware
{
    public:

        enum class Type : int
        {
            SENCRYPTION,
            DIGEST,
            MAC,
            HMAC,
            AEAD,
            PBKDF,
            SIGNATURE,
            AENCRYPTION,
            DH,
            ECDH
        };
        using Name=common::FixedByteArray256;

        //! Dtor
        virtual ~CryptAlgorithm()=default;

        CryptAlgorithm(const CryptAlgorithm&)=delete;
        CryptAlgorithm(CryptAlgorithm&&) =delete;
        CryptAlgorithm& operator=(const CryptAlgorithm&)=delete;
        CryptAlgorithm& operator=(CryptAlgorithm&&) =delete;

        /**
         * @brief Ctor
         * @param engine Backend crypt engine
         * @param type Algorithm type
         * @param name Name of algorithm encoding actual name of cryptographic algorithm in backend and optional set of algorithm parameters.
         *             E.g.: "RSA/2048", where RSA is algorithm name and 2048 is a parameter, standing for bits length in this case for RSA.
         * @param id ID of algorithm assigned by backend if applicable
         * @param native Native algorithm handler defined by backend
         */
        CryptAlgorithm(const CryptEngine* engine, Type type,const char* name,int id=0,const void* native=nullptr) noexcept
            : CryptEngineAware(engine),
              m_type(type),m_name(name),m_id(id),m_handler(native)
        {}

        //! Set native algorithm handler
        inline void setHandler(const void* handler) noexcept
        {
            m_handler=handler;
        }
        //! Get native algorithm handler
        inline const void* handler() const noexcept
        {
            return m_handler;
        }
        //! Is native algorithm handler valid
        bool isValid() const noexcept
        {
            return m_handler!=nullptr;
        }

        //! Reinterpret cast handler to native type
        template <typename T> const T* nativeHandler() const noexcept
        {
            return reinterpret_cast<const T*>(handler());
        }

        //! Set integer ID of algorithm assigned by backend
        inline void setID(int id) noexcept
        {
           m_id=id;
        }
        //! Det integer ID of algorithm assigned by backend
        inline int id() const noexcept
        {
            return m_id;
        }

        //! Get full name of algorithm including parameter string
        inline const char* name() const noexcept
        {
            return m_name.c_str();
        }

        //! Get algorithm type
        inline Type type() const noexcept
        {
            return m_type;
        }

        //! Cast algorithm to unsigned integer
        constexpr static uint32_t typeInt(Type type) noexcept
        {
            return static_cast<uint32_t>(type);
        }
        //! Check if algorithm can be used for specific type
        inline bool isType(Type t) const noexcept
        {
            return t==m_type;
        }

        //! Get key size of algorithm if applicable
        virtual size_t keySize() const
        {
            Assert(false,"Key size is undefined for this algorithm");
            return 0;
        }

        //! Get IV size of algorithm if applicable
        virtual size_t ivSize() const
        {
            Assert(false,"IV size is undefined for this algorithm");
            return 0;
        }

        //! Check if padding is enabled if applicable
        virtual bool enablePadding() const
        {
            Assert(false,"Padding is undefined for this algorithm");
            return false;
        }
        virtual void setEnablePadding(bool enable)
        {
            Assert(false,"Padding is undefined for this algorithm");
            std::ignore=enable;
        }

        //! Get hash size of algorithm if applicable
        virtual size_t hashSize(bool safe=false) const
        {
            if (!safe)
            {
                Assert(false,"Hash size is undefined for this algorithm");
            }
            return 0;
        }

        //! Get block size of algorithm if applicable
        virtual size_t blockSize() const
        {
            Assert(false,"Block size is undefined for this algorithm");
            return 0;
        }

        //! Get tag size of algorithm if applicable
        virtual size_t tagSize() const
        {
            Assert(false,"Tag size is undefined for this algorithm");
            return 0;
        }
        virtual void setTagSize(size_t size)
        {
            Assert(false,"Tag size is undefined for this algorithm");
            std::ignore=size;
        }

        virtual common::SharedPtr<MACKey> createMACKey() const;
        virtual common::SharedPtr<SymmetricKey> createSymmetricKey() const;
        virtual common::SharedPtr<PrivateKey> createPrivateKey() const;
        virtual common::SharedPtr<SignatureSign> createSignatureSign() const;
        virtual common::SharedPtr<SignatureVerify> createSignatureVerify() const;
        virtual common::SharedPtr<PublicKey> createPublicKey() const;

        virtual bool isBackendAlgorithm() const
        {
            return true;
        }

        virtual const char* paramStr(size_t index=0) const
        {
            std::ignore=index;
            return nullptr;
        }

        virtual bool isNone() const noexcept
        {
            return false;
        }

    private:

        Type m_type;
        common::FixedByteArray256 m_name;
        int m_id;
        const void* m_handler;
};
using CryptAlgorithmConstP=const CryptAlgorithm*;

//! Definition of map key {algorithm_name, engine_name} to use in maps of CryptAlgorithm
struct CryptAlgorithmMapKey
{
    common::FixedByteArray256 m_name;
    common::FixedByteArray256 m_engine;

    friend inline bool operator ==(const CryptAlgorithmMapKey& left,const CryptAlgorithmMapKey& right) noexcept
    {
        return left.m_name==right.m_name
                &&
               left.m_engine==right.m_engine
                ;
    }
    friend inline bool operator <(const CryptAlgorithmMapKey& left,const CryptAlgorithmMapKey& right) noexcept
    {
        if (left.m_name==right.m_name)
        {
            return left.m_engine<right.m_engine;
        }
        return left.m_name<right.m_name;
    }
};

//! Definition of map key {algorithm_type, algorithm_name} to use in maps of CryptEngine
struct CryptAlgorithmTypeNameMapKey
{
    CryptAlgorithm::Type m_type;
    CryptAlgorithm::Name m_name;

    friend inline bool operator ==(const CryptAlgorithmTypeNameMapKey& left,const CryptAlgorithmTypeNameMapKey& right) noexcept
    {
        return left.m_type==right.m_type
                &&
                left.m_name==right.m_name;
    }
    friend inline bool operator <(const CryptAlgorithmTypeNameMapKey& left,const CryptAlgorithmTypeNameMapKey& right) noexcept
    {
        int diff=static_cast<int>(left.m_type)-static_cast<int>(right.m_type);
        if (diff<0)
        {
            return true;
        }
        else if (diff>0)
        {
            return false;
        }
        return left.m_name<right.m_name;
    }
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTALG_H
