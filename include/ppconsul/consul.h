//  Copyright (c) 2014-2016 Andrey Upadyshev <oliora@gmail.com>
//
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "ppconsul/config.h"
#include "ppconsul/error.h"
#include "ppconsul/types.h"
#include "ppconsul/parameters.h"
#include "ppconsul/http_status.h"
#include "ppconsul/response.h"
#include "ppconsul/http_client.h"
#include <chrono>
#include <string>
#include <vector>
#include <tuple>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <limits>
#include <iostream>


namespace ppconsul {

    namespace keywords {
        PPCONSUL_PARAM(dc, std::string)
        PPCONSUL_PARAM(token, std::string)
        PPCONSUL_PARAM_NO_NAME(consistency, Consistency)
        PPCONSUL_PARAM_NO_NAME(block_for, BlockForValue)
        PPCONSUL_PARAM(tag, std::string)

        inline void printParameter(std::ostream& os, const Consistency& v, KWARGS_KW_TAG(consistency))
        {
            switch (v)
            {
                case Consistency::Stale: os << "stale"; break;
                case Consistency::Consistent: os << "consistent"; break;
                default: break;
            }
        }

        inline void printParameter(std::ostream& os, const BlockForValue& v, KWARGS_KW_TAG(block_for))
        {
            os << "wait=" << v.first.count() << "ms&index=" << v.second;
        }
    }

    const char Default_Server_Address[] = "127.0.0.1:8500";

    struct TlsSettings
    {
        // Params are c-strings because it's easiest way to allow user to store
        // the sensitive data somehow specially (e.g. locked and shreded after use memory).
        // User stores it in its way and give us only a pointer. And it's also compatible with cURL :)

        // More details about security options https://curl.haxx.se/libcurl/c/curl_easy_setopt.html

        // Note that Consul 0.1 supports only PKCS#12 and PEM format certificates
        // TODO: Check what's really supported on OSX?

        // Path or name of the client certificate file (CURLOPT_SSLCERT)
        const char *clientCert = nullptr;
        // Path or name of the client private key (CURLOPT_SSLKEY)
        const char *clientKey = nullptr;
        // Password for the client's private key or certificate file (CURLOPT_KEYPASSWD)
        const char *clientKeyPassword = nullptr;

        // Path to CA certificate bundle or dir or whatever -> need to be checked on osx/lnx/win!
        // (CURLOPT_CAINFO and/or CURLOPT_CAPATH???)
        const char *caCert = nullptr;

        // always set CURLOPT_SSL_FALSESTART to 1, ignore error
        // ?always set CURL_SSLVERSION_DEFAULT to CURL_SSLVERSION_TLSv1_1 or 1_2 (check what Consul supports)
        // ?always set CURLOPT_SSL_VERIFYSTATUS to 1, ignore error
    };

    class Consul
    {
    public:
        // Allowed parameters:
        // - dc - data center to use
        // - token - default token for all client requests (can be overloaded in every specific request)
        template<class... Params, class = kwargs::enable_if_kwargs_t<Params...>>
        explicit Consul(const std::string& addr, const Params&... params)
        : Consul(kwargs::get_opt(keywords::token, std::string(), params...),
                kwargs::get_opt(keywords::dc, std::string(), params...),
                addr)
        {
            KWARGS_CHECK_IN_LIST(Params, (keywords::dc, keywords::token))
        }

        // Same as Consul(Default_Server_Address, ...)
        // Allowed parameters:
        // - dc - data center to use
        // - token - default token for all requests
        template<class... Params, class = kwargs::enable_if_kwargs_t<Params...>>
        explicit Consul(const Params&... params)
            : Consul(Default_Server_Address, params...)
        {}

        Consul(Consul &&op) PPCONSUL_NOEXCEPT
        : m_client(std::move(op.m_client))
        , m_dataCenter(std::move(op.m_dataCenter))
        , m_defaultToken(std::move(op.m_defaultToken))
        {}

        Consul& operator= (Consul &&op) PPCONSUL_NOEXCEPT
        {
            m_client = std::move(op.m_client);
            m_dataCenter = std::move(op.m_dataCenter);
            m_defaultToken = std::move(op.m_defaultToken);

            return *this;
        }

        Consul(const Consul &op) = delete;
        Consul& operator= (const Consul &op) = delete;

        // Throws BadStatus if !response_status.success()
        template<class... Params, class = kwargs::enable_if_kwargs_t<Params...>>
        Response<std::string> get(WithHeaders, const std::string& path, const Params&... params);

        // Throws BadStatus if !response_status.success()
        template<class... Params, class = kwargs::enable_if_kwargs_t<Params...>>
        std::string get(const std::string& path, const Params&... params)
        {
            return std::move(get(withHeaders, path, params...).data());
        }

        template<class... Params, class = kwargs::enable_if_kwargs_t<Params...>>
        Response<std::string> get(http::Status& status, const std::string& path, const Params&... params)
        {
            return get_impl(status, makeUrl(path, params...));
        }

        template<class... Params, class = kwargs::enable_if_kwargs_t<Params...>>
        std::string put(http::Status& status, const std::string& path, const std::string& data, const Params&... params)
        {
            return put_impl(status, makeUrl(path, params...), data);
        }

        // Throws BadStatus if !response_status.success()
        template<class... Params, class = kwargs::enable_if_kwargs_t<Params...>>
        std::string put(const std::string& path, const std::string& data, const Params&... params);

        template<class... Params, class = kwargs::enable_if_kwargs_t<Params...>>
        std::string del(http::Status& status, const std::string& path, const Params&... params)
        {
            return del_impl(status, makeUrl(path, params...));
        }

        // Throws BadStatus if !response_status.success()
        template<class... Params, class = kwargs::enable_if_kwargs_t<Params...>>
        std::string del(const std::string& path, const Params&... params);

    private:
        Consul(const std::string& defaultToken, const std::string& dataCenter, const std::string& addr);

        template<class... Params, class = kwargs::enable_if_kwargs_t<Params...>>
        std::string makeUrl(const std::string& path, const Params&... params) const
        {
            using namespace keywords;
            return parameters::makeUrl(path, dc = m_dataCenter, token = m_defaultToken, params...);
        }

        // TODO: make impl funcs inline
        Response<std::string> get_impl(http::Status& status, const std::string& url);
        std::string put_impl(http::Status& status, const std::string& url, const std::string& data);
        std::string del_impl(http::Status& status, const std::string& url);

        std::unique_ptr<http::impl::Client> m_client;
        std::string m_dataCenter;

        std::string m_defaultToken;
    };

    // Implementation

    inline void throwStatusError(http::Status status, std::string data)
    {
        if (NotFoundError::Code == status.code())
        {
            throw NotFoundError{};
            //throw NotFoundError(std::move(status), std::move(data));
        }
        else
        {
            throw BadStatus(std::move(status), std::move(data));
        }
    }

    template<class... Params, class>
    inline Response<std::string> Consul::get(WithHeaders, const std::string& path, const Params&... params)
    {
        http::Status s;
        auto r = get(s, path, params...);
        if (!s.success())
            throwStatusError(std::move(s), std::move(r.data()));
        return r;
    }

    template<class... Params, class>
    inline std::string Consul::put(const std::string& path, const std::string& data, const Params&... params)
    {
        http::Status s;
        auto r = put(s, path, data, params...);
        if (!s.success())
            throwStatusError(std::move(s), std::move(r));
        return r;
    }

    template<class... Params, class>
    inline std::string Consul::del(const std::string& path, const Params&... params)
    {
        http::Status s;
        auto r = del(s, path, params...);
        if (!s.success())
            throwStatusError(std::move(s), std::move(r));
        return r;
    }

    inline Response<std::string> Consul::get_impl(http::Status& status, const std::string& url)
    {
        Response<std::string> r;
        std::tie(status, r.headers(), r.data()) = m_client->get(url);
        return r;
    }

    inline std::string Consul::put_impl(http::Status& status, const std::string& url, const std::string& data)
    {
        std::string r;
        std::tie(status, r) = m_client->put(url, data);
        return r;
    }

    inline std::string Consul::del_impl(http::Status& status, const std::string& url)
    {
        std::string r;
        std::tie(status, r) = m_client->del(url);
        return r;
    }

}
