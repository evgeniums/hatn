/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/file.h
  *
  *      Class for files writing and reading
  *
  */

/****************************************************************************/

#ifndef HATNFILE_H
#define HATNFILE_H

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

#include <boost/beast/core/file.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <hatn/common/common.h>
#include <hatn/common/error.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Base class for files writing and reading
class File
{
    public:

        using Mode = boost::beast::file_mode;

        File()=default;
        virtual ~File()=default;
        File(const File&)=default;
        File(File&&) =default;
        File& operator=(const File&)=default;
        File& operator=(File&&) =default;

        /**
         * @brief Set filename
         * @param filename File name
         */
        inline void setFilename(std::string&& filename) noexcept
        {
            m_filename=std::move(filename);
        }
        /**
         * @brief Set filename
         * @param filename File name
         */
        template <typename ContainerT>
        inline void setFilename(const ContainerT& filename)
        {
            m_filename=std::string(filename.data(),filename.size());
        }
        /**
         * @brief Set filename
         * @param filename File name
         */
        inline void setFilename(const char* filename)
        {
            m_filename=filename;
        }
        /**
         * @brief Get filename
         */
        inline const std::string& filename() const noexcept
        {
            return m_filename;
        }
        /**
         * @brief Check if filename is empty
         */
        inline bool isFilenameEmpty() const noexcept
        {
            return m_filename.empty();
        }

        /**
         * @brief Open file
         * @param filename File name
         * @param mode Mode to open with, @see boost::beast::file_mode
         * @return Operation status
         */
        virtual Error open(const char* filename, Mode mode)=0;
        /**
         * @brief Open file
         * @param filename File name
         * @param mode Mode to open with, @see boost::beast::file_mode
         * @param copyFilename If true then keep file name
         * @return Operation status
         */
        Error open(const char* filename, Mode mode, bool copyFilename)
        {
            if (copyFilename)
            {
                setFilename(filename);
            }
            return open(filename,mode);
        }
        /**
         * @brief Open file
         * @param filename File name
         * @param mode Mode to open with, @see boost::beast::file_mode
         * @param copyFilename If true then keep file name
         * @return Operation status
         */
        template <typename ContainerT>
        Error open(const ContainerT& filename, Mode mode, bool copyFilename=false)
        {
            if (copyFilename)
            {
                setFilename(filename);
            }
            return open(filename.c_str(),mode);
        }
        /**
         * @brief Open file
         * @param mode Mode to open with, @see boost::beast::file_mode
         * @return Operation status
         */
        Error open(Mode mode)
        {
            return open(m_filename.c_str(),mode);
        }
        /**
         * @brief Open file
         * @param filename File name
         * @param mode Mode to open with, @see boost::beast::file_mode
         * @return Operation status
         */
        Error open(std::string&& filename, Mode mode)
        {
            setFilename(std::move(filename));
            return open(mode);
        }

        //! Check if the file is open
        virtual bool isOpen() const noexcept=0;

        //! Flush buffers to disk
        virtual Error flush() noexcept=0;

        //! Sync buffers to disk
        virtual Error sync() noexcept
        {
            return OK;
        }

        //! Fsync buffers to disk
        virtual Error fsync() noexcept
        {
            return OK;
        }

        /**
         * @brief Close file
         * @throws ErrorException on error
         */

        virtual void close()=0;
        /**
         * @brief Close file
         * @param ec Error if opening failed
         */

        void close(Error& ec) noexcept
        {
            ec.reset();
            try
            {
                return close();
            }
            catch (const ErrorException& e)
            {
                ec=e.error();
            }
        }

        /**
         * @brief Set cursor position in file
         * @param pos Position
         * @return Operation status
         */
        virtual Error seek(uint64_t pos)=0;
        /**
         * @brief Get current cursor position in file
         * @throws ErrorException on error
         */
        virtual uint64_t pos() const=0;
        /**
         * @brief Get current cursor position in file
         * @param ec Error if operation failed
         */
        uint64_t pos(Error& ec) const noexcept
        {
            ec.reset();
            try
            {
                return pos();
            }
            catch (const ErrorException& e)
            {
                ec=e.error();
            }
            return 0u;
        }

        /**
         * @brief Get size of file content
         * @throws ErrorException if operation failed
         */
        virtual uint64_t size() const=0;

        /**
         * @brief Get size of file content
         * @param ec Error if operation failed
         */
        virtual uint64_t size(Error& ec) const=0;

        /**
         * @brief Get size of file on disk
         * @throws ErrorException if operation failed
         */
        virtual uint64_t storageSize() const
        {
            return size();
        }
        /**
         * @brief Get size of file on disk
         * @param ec Error if operation failed
         */
        uint64_t storageSize(Error& ec) const
        {
            ec.reset();
            try
            {
                return storageSize();
            }
            catch (const ErrorException& e)
            {
                ec=e.error();
            }
            return 0u;
        }

        /**
         * @brief Write data to file
         * @param data Data buffer
         * @param size Data size
         * @return Written size
         * @throws ErrorException if operation failed
         */
        virtual size_t write(const char* data, size_t size)=0;
        /**
         * @brief Write data to file
         * @param data Data buffer
         * @param size Data size
         * @param ec Error if operation failed
         * @return Written size
         */
        size_t write(const char* data, size_t size, Error& ec) noexcept
        {
            ec.reset();
            try
            {
                return write(data,size);
            }
            catch (const ErrorException& e)
            {
                ec=e.error();
            }
            return 0u;
        }

        /**
         * @brief Read data from file
         * @param data Target data buffer
         * @param maxSize Max size to read
         * @return Read size
         * @throws ErrorException if operation failed
         */
        virtual size_t read(char* data, size_t maxSize)=0;
        /**
         * @brief Read data from file
         * @param data Target data buffer
         * @param maxSize Max size to read
         * @param ec Error if operation failed
         * @return Read size
         */
        size_t read(char* data, size_t maxSize, Error& ec) noexcept
        {
            ec.reset();
            try
            {
                return read(data,maxSize);
            }
            catch (const ErrorException& e)
            {
                ec=e.error();
            }
            return 0u;
        }

        /**
         * @brief Read whole file to container.
         * @param container.
         * @return Operation status.
         *
         * Can read only open file.
         */
        template <typename ContainerT>
        Error readAll(ContainerT& container)
        {
            try
            {
                auto fileSize=size();
                container.resize(static_cast<size_t>(fileSize));
                seek(0);
                auto readSize=read(container.data(),static_cast<size_t>(fileSize));
                if (readSize!=static_cast<size_t>(fileSize))
                {
                    throw ErrorException(commonError(CommonError::FILE_READ_FAILED));
                }
            }
            catch (const ErrorException& e)
            {
                container.clear();
                return e.error();
            }
            return OK;
        }

        /**
         * @brief Read whole file to container.
         * @param container.
         * @return Operation status.
         *
         * Can read only not open file.
         */
        template <typename FilenameT, typename ContainerT>
        Error readAll(const FilenameT& filename, ContainerT& container)
        {
            HATN_CHECK_RETURN(open(filename,Mode::scan))
            auto ec=readAll(container);
            Error ec1;
            close(ec1);
            return ec;
        }

        /**
         * @brief Truncate file.
         * @param size New size. If new size is greater than current size then noop.
         * @param backupCopy Make a backup copy to restore the file in case of error.
         * @return Operation status.
        */
        virtual common::Error truncate(size_t /*size*/, bool /*backupCopy*/)
        {
            return CommonError::NOT_IMPLEMENTED;
        }

    private:

        std::string m_filename;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNFILE_H
