//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "connection.hpp"
#include <vector>
#include "request_handler.hpp"
#include <osg/Notify>

namespace http {
namespace server {

connection::connection(asio::io_service& io_service,
    request_handler& handler)
  : socket_(io_service),
    request_handler_(handler)
{
    OSG_DEBUG << "RestHttpDevice :: connection::connection" << std::endl;
}

connection::~connection()
{
    OSG_DEBUG << "RestHttpDevice :: connection::~connection" << std::endl;
}
asio::ip::tcp::socket& connection::socket()
{
  return socket_;
}

void connection::start()
{
  OSG_DEBUG << "RestHttpDevice :: connection::start" << std::endl;
  
  socket_.async_read_some(asio::buffer(buffer_),
      std::bind(&connection::handle_read, shared_from_this(),
        std::placeholders::_1,
        std::placeholders::_2));
}

void connection::handle_read(const asio::error_code& e,
    std::size_t bytes_transferred)
{
  if (!e)
  {
    boost::tribool result;
    boost::tie(result, boost::tuples::ignore) = request_parser_.parse(
        request_, buffer_.data(), buffer_.data() + bytes_transferred);

    if (result)
    {
      request_handler_.handle_request(request_, reply_);
      asio::async_write(socket_, reply_.to_buffers(),
          std::bind(&connection::handle_write, shared_from_this(),
            asio::placeholders::_1));
    }
    else if (!result)
    {
      reply_ = reply::stock_reply(reply::bad_request);
      asio::async_write(socket_, reply_.to_buffers(),
          std::bind(&connection::handle_write, shared_from_this(),
            asio::placeholders::_1));
    }
    else
    {
      socket_.async_read_some(asio::buffer(buffer_),
          std::bind(&connection::handle_read, shared_from_this(),
            asio::placeholders::_1,
            asio::placeholders::_2));
    }
  }

  // If an error occurs then no new asynchronous operations are started. This
  // means that all shared_ptr references to the connection object will
  // disappear and the object will be destroyed automatically after this
  // handler returns. The connection class's destructor closes the socket.
}

void connection::handle_write(const asio::error_code& e)
{
  if (!e)
  {
    // Initiate graceful connection closure.
    asio::error_code ignored_ec;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
  }

  // No new asynchronous operations are started. This means that all shared_ptr
  // references to the connection object will disappear and the object will be
  // destroyed automatically after this handler returns. The connection class's
  // destructor closes the socket.
}

} // namespace server
} // namespace http
