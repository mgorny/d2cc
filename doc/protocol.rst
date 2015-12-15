========================================================================
   d2cc protocol specification (draft)
========================================================================

Introduction
============

This document specifies the protocol used by various d2cc components.
The specification is split into separate sections covering basic data
flow for different cases of d2cc communications, and a detailed
reference of particular data packets.

The protocol is a streaming, binary protocol. All messages are encoded
directly into the data stream with no explicit delimiters. Contents of
different messages can not be interspersed. Unless stated otherwise,
the messages can be sent in any order.

All messages start with a header indicating the message type and length.
In some cases, multiple messages of the same type are allowed. If that
is the case, the ordering of messages is meaningful and the messages
must be processed in sequence.

All integers are encoded using the network byte order (big endian). All
character data are encoded using the UTF-8 encoding. Strings explicitly
noted as null-terminated are terminated by a NUL character and must not
contain any NUL characters in their contents. Other strings and binary
data are stored undelimited.

All communications start with the request-side hello packet, consisting
of the D2CC magic bytes and the protocol version. Upon receiving invalid
magic bytes, the connection must be terminated immediately with no
reply. Upon receiving unsupported protocol version, an error must be
sent in a supported protocol version and the connection must be
terminated.


Communication scenarios
=======================

toolchain wrapper to daemon communications
------------------------------------------

The communication between toolchain wrappers and the daemon is performed
using a local UNIX socket. The socket path is defined at build time.
The socket is created and listened on by the daemon.

The toolchain wrapper (client) establishes the connection to the UNIX
socket, and upon the connection being accepted:

1. client sends the hello message,

2. client sends the compilation request messages,

3. client shutdowns the write end of the connection,

4. server sends the hello message,

5. server replies with compilation response messages,

6. the connection is terminated.

It should be noted that the hello message is immediately followed by
compilation request. Unless in an error condition, the server does not
reply before receiving the complete request. The server must gracefully
accept the complete request even when responding with an error message.


Message format reference
========================

Message header
--------------

Each message starts with a message header that specifies the message
type and length.

The format of the header is outlined as::

    0                                31
    +---------------------------------+
    |           message type          |
    +---------------------------------+
    |          message length         |
    +---------------------------------+

Message type is a 32-bit (4 octets) binary string matching one of
pre-defined message type constants. It is an error to use a message type
not defined by the protocol version announced in the hello message. Upon
receiving an invalid message type, the peer must report a protocol error
and terminate the connection. In this case, the peer must not use
the message length field or attempt processing the message.

Message length is a 32-bit unsigned integer. It defines the length
in octets of the complete message (including the header).
If the connection is terminated before receiving the specified number of
octets, the message must not be processed.


Basic messages
--------------

Hello message
~~~~~~~~~~~~~

The hello message is the first message that is sent by both parties
in each connection. The messages defines and confirms the used protocol,
therefore providing means both for verifying the compatibility between
the peers and ensuring that the correct service is being connected to.

The format of the hello message is outlined as::

    0                                31
    +---------------------------------+
    |           "D2CC" magic          |
    +---------------------------------+
    |          message length         |
    +----------------+----------------+
    |    version     |      flags     |
    +----------------+----------------+

The message type is set to "D2CC". The message size is equal to 12
octets. Upon receiving initial octets not matching those, the peer must
terminate the connection without any reply.

Version is a 16-bit unsigned integer specifying the protocol version.
The current protocol version is 0x0000. Upon receiving a hello message
with an unsupported protocol version, the peer should issue a hello
message in a supported (lower) protocol version, followed by
an incompatibility error and terminate the connection.

Flags is a 16-bit flag field. Meaning of flag bits is defined for
a particular protocol version. Flag bits unused in a particular protocol
version can be reused without increasing protocol version. Upon
receiving flags with unrecognized bits set, the peer should issue
an incompatibility error and terminate the connection.

No flags are defined at the moment.


DSCR -- discard request message
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The discard request message is sent whenever a client-side error occurs
after transmitting at least part of the request. Upon receiving this
message, the peer must discard all previous messages in the request,
and abort the request processing if it already started. Afterwards,
a new request may follow.

It should be noted that if discard request is sent after the request
confirmation message it may or may not terminate the request
in-progress. The peer must be prepared both to receive discard
confirmation and a regular response to the request.

The message type is set to "DSCR". The message size is equal to 8.
The message has no body.


Compilation request messages
----------------------------

The request
~~~~~~~~~~~

A compilation request is formed using the following messages:

- one "ARGV" message defining the compiler command-line,

- one or more "DATA" messages passing the preprocessed sources,

- exactly one "CREQ" message completing the compilation request.

The "CREQ" message must be submitted as a last message in the request.
The remaining messages can be passed in any order, and interspersed.


ARGV -- compiler command-line
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The "ARGV" messages are used to transmit the command-line arguments that
will be used to spawn the compiler. If multiple "ARGV" messages are
transmitted, their data will be concatenated to form the final command.

The message type is set to "ARGV". The message length defines the number
of data bytes following the header, increased by the header size.

The header is immediately followed by data bytes. The concatenated data
bytes form a list of null-terminated strings corresponding to
command-line arguments, starting with program name.

Note: the last argument must be null-terminated as well.

The arguments must contain at least one occurrence of each of
the following placeholders:

- `@d2cc_input@` for compiler input file name,

- `@d2cc_output@` for compiler output file name.

For example, the following command line::

    gcc -c -o @d2cc_output@ @d2cc_input@

is encoded as::

    gcc\0-c\0-o\0@d2cc_output@\0@d2cc_input@\0


DATA -- pre-processed program data
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The "DATA" messages are used to transmit the compiler pre-processing
results that will be passed to the compiler. If multiple "DATA" messages
are sent, their data will be concatenated in order.

The message type is set to "DATA". The message length defines the number
of data bytes following the header, increased by the header size.

The header is immediately followed by data bytes. The data contains
the pre-processor output as undelimitered string. The end of last DATA
message implies the end of data.


CREQ -- compile request confirmation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The "CREQ" message confirms the compile request. It must be sent after
all other request data messages in order to start processing.

The message type is set to "CREQ". The message size is equal to 8.
The message has no body.
