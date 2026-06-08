/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/clientappfilesettings.h
  *
  * Application settings stored as a JSON file with immediate synchronous persistence.
  */

/****************************************************************************/

#ifndef HATNCLIENTAPPFILESETTINGS_H
#define HATNCLIENTAPPFILESETTINGS_H

#include <functional>
#include <string>

#include <hatn/common/locker.h>

#include <hatn/base/configtree.h>
#include <hatn/base/configtreevalue.h>

#include <hatn/clientapp/clientappdefs.h>
#include <hatn/clientapp/clientappsettings.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class ClientApp;

/**
 * @brief File-backed settings persisted as JSON.
 *
 * Unlike ClientAppSettings (which uses the DB and async flush), this class
 * writes every change to a JSON file synchronously using FileUtils::saveToFileWithBackup.
 *
 * Two accessor families:
 *  - Internally-locked: set(), apply(), save(), getAs*() — thread-safe, no external locking needed.
 *  - Unguarded: configTreeUnguarded(), getUnguarded(), getArrayUnguarded(), getMapUnguarded(),
 *    toArrayUnguarded(), toMapUnguarded() — caller must hold lock()/unlock().
 */
class HATN_CLIENTAPP_EXPORT ClientAppFileSettings
{
    public:

        constexpr static const char* FileName = "filesettings.json";

        using Mutator = std::function<Error(HATN_BASE_NAMESPACE::ConfigTree&)>;

        explicit ClientAppFileSettings(ClientApp* app);

        virtual ~ClientAppFileSettings() = default;
        ClientAppFileSettings(const ClientAppFileSettings&) = delete;
        ClientAppFileSettings(ClientAppFileSettings&&) = delete;
        ClientAppFileSettings& operator=(const ClientAppFileSettings&) = delete;
        ClientAppFileSettings& operator=(ClientAppFileSettings&&) = delete;

        /** @brief Load settings from file. A missing file is treated as empty settings (returns OK). */
        Error load(const std::string& filePath);

        //-----------------------------------------------------------------------
        // Write API
        //-----------------------------------------------------------------------

        /**
         * @brief Canonical write path — atomic, works for scalars, vectors, maps and multi-field edits.
         *
         * Locks, runs the mutator against the raw ConfigTree (use toArray/toMap for containers,
         * multiple set calls for multi-field edits, etc.), then serializes+writes to file.
         * If the mutator returns an error the tree is NOT written. Publishes AppSettingsEvent on success.
         */
        Error apply(Mutator mutator, std::string section = "");

        /**
         * @brief Set a scalar value. Thin wrapper over apply(); does NOT work for vectors/maps.
         *
         * For non-scalar containers use apply() or toArrayUnguarded()/toMapUnguarded() + save().
         */
        template <typename T>
        Error set(const HATN_BASE_NAMESPACE::ConfigTreePath& path, T&& value, std::string section = "")
        {
            return apply(
                [&](HATN_BASE_NAMESPACE::ConfigTree& tree)
                {
                    auto r = tree.set(path, std::forward<T>(value));
                    return r ? r.takeError() : Error{};
                },
                std::move(section)
            );
        }

        /**
         * @brief Flush escape hatch: serialize+write the current tree state.
         *
         * Use after mutating containers through the Unguarded accessors under lock()/unlock().
         * Caller must NOT hold the lock when calling save() — it re-acquires m_mutex internally.
         * For fully atomic container updates prefer apply().
         */
        Error save(std::string section = "");

        //-----------------------------------------------------------------------
        // Locking primitives (required before using any Unguarded accessor)
        //-----------------------------------------------------------------------

        void lock() const
        {
            m_mutex.lock();
        }

        void unlock() const
        {
            m_mutex.unlock();
        }

        common::MutexLock& mutex()
        {
            return m_mutex;
        }

        //-----------------------------------------------------------------------
        // Unguarded accessors — caller must hold lock()/unlock()
        //-----------------------------------------------------------------------

        const HATN_BASE_NAMESPACE::ConfigTree& configTreeUnguarded() const noexcept
        {
            return m_configTree;
        }

        HATN_BASE_NAMESPACE::ConfigTree& configTreeUnguarded() noexcept
        {
            return m_configTree;
        }

        /** @brief Generic subtree read. Returns reference into internal storage. */
        Result<const HATN_BASE_NAMESPACE::ConfigTree&> getUnguarded(
            const HATN_BASE_NAMESPACE::ConfigTreePath& path
        ) const noexcept
        {
            return m_configTree.get(path);
        }

        /**
         * @brief Read-only array view.
         * The view is valid only while the caller holds the lock.
         */
        template <typename T>
        Result<HATN_BASE_NAMESPACE::ConstArrayView<T>> getArrayUnguarded(
            const HATN_BASE_NAMESPACE::ConfigTreePath& path
        ) const
        {
            auto r = m_configTree.get(path);
            HATN_CHECK_RESULT(r)
            return r.value().template asArray<T>();
        }

        /**
         * @brief Read-only map reference.
         * The reference is valid only while the caller holds the lock.
         */
        Result<const HATN_BASE_NAMESPACE::config_tree::MapT&> getMapUnguarded(
            const HATN_BASE_NAMESPACE::ConfigTreePath& path
        ) const
        {
            auto r = m_configTree.get(path);
            HATN_CHECK_RESULT(r)
            return r.value().asMap();
        }

        /**
         * @brief Mutable array builder — auto-creates path.
         * Use under lock(); call save() afterwards to persist.
         */
        template <typename T>
        Result<HATN_BASE_NAMESPACE::ArrayView<T>> toArrayUnguarded(
            const HATN_BASE_NAMESPACE::ConfigTreePath& path
        )
        {
            return m_configTree.toArray<T>(path);
        }

        /**
         * @brief Mutable map builder — auto-creates path.
         * Use under lock(); call save() afterwards to persist.
         */
        Result<HATN_BASE_NAMESPACE::config_tree::MapT&> toMapUnguarded(
            const HATN_BASE_NAMESPACE::ConfigTreePath& path
        )
        {
            return m_configTree.toMap(path);
        }

        //-----------------------------------------------------------------------
        // Internally-locked typed value-copy getters — no external lock needed
        //-----------------------------------------------------------------------

        /**
         * @brief Get a copy of the value at path. Mirrors ConfigTreeValue::as<T>().
         *
         * Always returns a value copy so the result is safe after the internal lock is released.
         * Suitable types: bool, integer types, double, std::string.
         */
        template <typename T>
        Result<std::decay_t<T>> getAs(
            const HATN_BASE_NAMESPACE::ConfigTreePath& path
        ) const
        {
            using RetT = std::decay_t<T>;
            common::MutexScopedLock l{m_mutex};
            auto r = m_configTree.get(path);
            HATN_CHECK_RESULT(r)
            auto v = r.value().template as<T>();
            if (v) return v.takeError();
            return RetT{v.value()};  // copy before lock is released
        }

        /** @brief Get value, reporting error via out-param. Mirrors ConfigTreeValue::as<T>(Error&). */
        template <typename T>
        auto getAs(
            const HATN_BASE_NAMESPACE::ConfigTreePath& path,
            common::Error& ec
        ) const -> std::decay_t<T>
        {
            auto r = getAs<T>(path);
            HATN_RESULT_EC(r, ec)
            return r.takeValue();
        }

        /** @brief Get value, throwing on error. Mirrors ConfigTreeValue::asEx<T>(). */
        template <typename T>
        auto getAsEx(
            const HATN_BASE_NAMESPACE::ConfigTreePath& path
        ) const -> std::decay_t<T>
        {
            auto r = getAs<T>(path);
            HATN_RESULT_THROW(r)
            return r.takeValue();
        }

        // --- Concrete convenience getters (defined in clientappfilesettings.cpp) ---

        Result<std::string> getAsString(const HATN_BASE_NAMESPACE::ConfigTreePath& path) const;
        Result<bool>        getAsBool(const HATN_BASE_NAMESPACE::ConfigTreePath& path) const;
        Result<double>      getAsDouble(const HATN_BASE_NAMESPACE::ConfigTreePath& path) const;
        Result<int64_t>     getAsInt(const HATN_BASE_NAMESPACE::ConfigTreePath& path) const;
        Result<uint64_t>    getAsUInt(const HATN_BASE_NAMESPACE::ConfigTreePath& path) const;

        std::string getAsString(const HATN_BASE_NAMESPACE::ConfigTreePath& path, common::Error& ec) const;
        bool        getAsBool(const HATN_BASE_NAMESPACE::ConfigTreePath& path, common::Error& ec) const;
        double      getAsDouble(const HATN_BASE_NAMESPACE::ConfigTreePath& path, common::Error& ec) const;
        int64_t     getAsInt(const HATN_BASE_NAMESPACE::ConfigTreePath& path, common::Error& ec) const;
        uint64_t    getAsUInt(const HATN_BASE_NAMESPACE::ConfigTreePath& path, common::Error& ec) const;

        // --- JSON-serialized accessors (for the C bridge / whitemclientbridge) ---

        /**
         * @brief Set a non-scalar subsection from a JSON string.
         *
         * Parses @a json into a temporary ConfigTree first (outside the lock), so invalid JSON
         * can never corrupt the live tree. On success, atomically replaces the subsection at @a path
         * (remove then merge), flushes to file, and publishes AppSettingsEvent(@a section).
         */
        Error setJson(
            const HATN_BASE_NAMESPACE::ConfigTreePath& path,
            const std::string& json,
            std::string section = ""
        );

        /**
         * @brief Get a subsection as a serialized JSON string.
         *
         * Returns APPLICATION_SETTING_NOT_SET if the path has no value.
         */
        Result<std::string> getJson(const HATN_BASE_NAMESPACE::ConfigTreePath& path) const;

        /** @brief Get a subsection as a serialized JSON string, reporting error via out-param. */
        std::string getJson(const HATN_BASE_NAMESPACE::ConfigTreePath& path, common::Error& ec) const;

    private:

        /** @brief Serialize and write to file. Must be called with m_mutex held. */
        Error saveLocked();

        /** @brief Publish AppSettingsEvent. Must be called WITHOUT m_mutex held. */
        void publishChange(std::string section);

        ClientApp* m_app;
        std::string m_filePath;
        HATN_BASE_NAMESPACE::ConfigTree m_configTree;
        mutable common::MutexLock m_mutex;
};

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNCLIENTAPPFILESETTINGS_H
