/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/plainfile.h
  *
  *      Class for writing and reading plain files
  *
  */

/****************************************************************************/

#ifndef HATNPLAINFILE_H
#define HATNPLAINFILE_H

#include <hatn/common/file.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Class for writing and reading plain files
class HATN_COMMON_EXPORT PlainFile : public File
{
    public:

        using File::File;
        using File::open;
        using File::close;
        using File::pos;
        using File::size;
        using File::write;
        using File::read;
        using File::storageSize;

        virtual ~PlainFile();
        PlainFile(const PlainFile&)=delete;
        PlainFile(PlainFile&&) =delete;
        PlainFile& operator=(const PlainFile&)=delete;
        PlainFile& operator=(PlainFile&&) =delete;

        /**
         * @brief Open file
         * @param filename File name
         * @param mode Mode to open with, @see boost::beast::file_mode
         * @return Operation status
         */
        virtual Error open(const char* filename, Mode mode) override;

        //! Check if the file is open
        virtual bool isOpen() const noexcept override;

        //! Flush buffers to disk
        virtual Error flush() noexcept override;

        /**
         * @brief Close file
         * @throws ErrorException on error
         */
        virtual void close() override;

        /**
         * @brief Set cursor position in file
         * @param pos Position
         * @return Operation status
         */
        virtual Error seek(uint64_t pos) override;

        /**
         * @brief Get current cursor position in file
         * @throws ErrorException on error
         */
        virtual uint64_t pos() const override;

        /**
         * @brief Get size of file content
         * @throws ErrorException if operation failed
         */
        virtual uint64_t size() const override;

        /**
         * @brief Write data to file
         * @param data Data buffer
         * @param size Data size
         * @return Written size
         * @throws ErrorException if operation failed
         */
        virtual size_t write(const char* data, size_t size) override;

        /**
         * @brief Read data from file
         * @param data Target data buffer
         * @param maxSize Max size to read
         * @return Read size
         * @throws ErrorException if operation failed
         */
        virtual size_t read(char* data, size_t maxSize) override;

    private:

        void doClose();

        boost::beast::file m_file;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNPLAINFILE_H
