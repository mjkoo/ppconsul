//  Copyright (c) 2014-2016 Andrey Upadyshev <oliora@gmail.com>
//
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <ppconsul/http_client.h>
#include <boost/network/protocol/http/client.hpp>


namespace ppconsul { namespace netlib {
    
    class HttpClient: public ppconsul::http::impl::Client
    {
    public:
        HttpClient(const std::string& address);

        virtual std::tuple<http::Status, ResponseHeaders, std::string> get(const std::string& path) override;
        virtual std::pair<http::Status, std::string> put(const std::string& path, const std::string& data) override;
        virtual std::pair<http::Status, std::string> del(const std::string& path) override;

    private:
        std::string makeUrl(const std::string& path) const { return m_addr + path; }

        std::string m_addr;
        boost::network::http::client m_client;
    };

}}
