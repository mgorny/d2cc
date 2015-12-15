// d2cc: UNIX socket client connection library
// (c) 2015 Michał Górny
// Distributed under the terms of the 2-clause BSD license

#pragma once

#ifndef D2CC_UNIXCLIENT_H
#define D2CC_UNIXCLIENT_H 1

#include <cstdint>
#include <functional>

#include "protocol.h"

namespace d2cc
{
	// UNIX socket client
	class UNIXClient
	{
		int socket_;

		// are we in request?
		bool in_request_;
		// how many messages do we wait to discard?
		int discard_msgs_;

		// send data to the socket
		// returns true on success, false otherwise
		bool send(const d2cc::protocol::buffer_type& data);

	public:
		UNIXClient();
		~UNIXClient();

		// connect to the d2cc socket
		// returns true on success
		// outputs error to stderr and returns false on failure
		bool connect();

		// send compiler argv to the server
		// calls user_func() to get argv
		//
		// the function should have signature like;
		//	void argv_getter(const d2cc::protocol::ArgVMessage::argv_yield_func& yield)
		// and call:
		//	yield(argv[i])
		// for each argv argument to send
		//
		// returns true on success, false on failure with stderr error
		bool send_argv(const d2cc::protocol::ArgVMessage::argv_getter_func& user_func);

		// send preprocessed file data to the server
		// calls data_read() to read data
		//
		// the function should have signature like:
		//	size_t data_read(uint8_t* data, size_t data_len)
		// for each call, it should fill data with at most data_len
		// bytes and return the amount of data read, 0 for EOF or -1
		// on error
		//
		// returns true on success, false on failure with stderr error
		bool send_data(const d2cc::protocol::DataMessage::data_read_func& data_read);

		// send final request packet to the server
		//
		// returns true on success, false on failure with stderr error
		bool finish_request();

		// send discard request message to the server
		// this effectively discards all data sent before
		//
		// returns true on success, false on failure with stderr error
		bool discard_request();
	};
};

#endif /*D2CC_UNIXCLIENT_H*/
