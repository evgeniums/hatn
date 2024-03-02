/*
 *  sha1.h
 *
 *  Copyright (C) 1998, 2009
 *  Paul E. Jones <paulej@packetizer.com>
 *  All Rights Reserved.
 *
 *  www.packetizer.com/security/sha1/sha1-c.zip
 *
    Copyright (C) 1998, 2009
    Paul E. Jones <paulej@packetizer.com>

    Freeware Public License (FPL)

    This software is licensed as "freeware."  Permission to distribute
    this software in source and binary forms, including incorporation
    into other products, is hereby granted without a fee.  THIS SOFTWARE
    IS PROVIDED 'AS IS' AND WITHOUT ANY EXPRESSED OR IMPLIED WARRANTIES,
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
    AND FITNESS FOR A PARTICULAR PURPOSE.  THE AUTHOR SHALL NOT BE HELD
    LIABLE FOR ANY DAMAGES RESULTING FROM THE USE OF THIS SOFTWARE, EITHER
    DIRECTLY OR INDIRECTLY, INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA
    OR DATA BEING RENDERED INACCURATE.

    2020 Edited by Evgeny Sidorov (decfile.com) by adding HATN_COMMON_EXPORT

 *****************************************************************************
 *  
 *****************************************************************************
 *
 *  Description:
 *      This class implements the Secure Hashing Standard as defined
 *      in FIPS PUB 180-1 published April 17, 1995.
 *
 *      Many of the variable names in this class, especially the single
 *      character names, were used because those were the names used
 *      in the publication.
 *
 *      Please read the file sha1.cpp for more information.
 *
 */

#ifndef _SHA1_H_
#define _SHA1_H_

#include <stdexcept>
#include <string>
#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

class HATN_COMMON_EXPORT SHA1 final
{
    public:

        SHA1();

        ~SHA1();
        SHA1(const SHA1&)=default;
        SHA1(SHA1&&) =default;
        SHA1& operator=(const SHA1&)=default;
        SHA1& operator=(SHA1&&) =default;

        /*
         * Calculate sha1 on file
         */
        static bool fileHash(const std::string& fileName, std::string& hexHash);

        /**
         * Calculate sha1 on container
         */
        template <typename ContainerT>
        static std::string containerHash(const ContainerT& container)
        {
            SHA1 sha1;
            sha1.Input(container.data(),static_cast<unsigned int>(container.size()));
            std::string hexHash;
            if (!sha1.Result(hexHash))
            {
                throw std::runtime_error("Failed to calculate sha1");
            }
            return hexHash;
        }

        /*
         *  Re-initialize the class
         */
        void Reset();

        /*
         *  Returns the message digest
         */
        bool Result(unsigned *message_digest_array);

        /*
         *  Returns the message digest in hex format
         */
        bool Result(std::string& hexHash);

        /*
         *  Provide input to SHA1
         */
        void Input( const unsigned char *message_array,
                    unsigned            length);
        void Input( const char  *message_array,
                    unsigned    length);
        void Input(unsigned char message_element);
        void Input(char message_element);
        SHA1& operator<<(const char *message_array);
        SHA1& operator<<(const unsigned char *message_array);
        SHA1& operator<<(const char message_element);
        SHA1& operator<<(const unsigned char message_element);

    private:

        /*
         *  Process the next 512 bits of the message
         */
        void ProcessMessageBlock();

        /*
         *  Pads the current message block to 512 bits
         */
        void PadMessage();

        /*
         *  Performs a circular left shift operation
         */
        inline unsigned CircularShift(int bits, unsigned word);

        unsigned H[5];                      // Message digest buffers

        unsigned Length_Low;                // Message length in bits
        unsigned Length_High;               // Message length in bits

        unsigned char Message_Block[64];    // 512-bit message blocks
        int Message_Block_Index;            // Index into message block array

        bool Computed;                      // Is the digest computed?
        bool Corrupted;                     // Is the message digest corruped?
    
};

}
}
#endif
