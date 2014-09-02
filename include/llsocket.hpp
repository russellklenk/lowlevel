/*/////////////////////////////////////////////////////////////////////////////
/// @summary Defines the runtime interface to the system sockets library; on
/// Windows the WinSock2 API is used while OSX/Linux uses the BSD sockets API.
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LLSOCKET_HPP_INCLUDED
#define LLSOCKET_HPP_INCLUDED

/*////////////////
//   Includes   //
////////////////*/
#if  defined(_WIN32) || defined(_WIN64)      /* WinSock2 sockets API     */
    #undef  _WIN32_WINNT
    #define _WIN32_WINNT _WIN32_WINNT_VISTA
    #include <stddef.h>
    #include <stdint.h>
    #include <errno.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #undef  _WIN32_WINNT
#else                                        /* standard BSD sockets API */
    #include <stddef.h>
    #include <stdint.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <arpa/inet.h>
    #include <netdb.h>
#endif

/*////////////////////
//   Preprocessor   //
////////////////////*/
/// @summary Abstract away Windows 32-bit calling conventions.
#if defined(_WIN32) && defined(_MSC_VER)
    #define  LLSOCKET_CALL_C    __cdecl
#else
    #define  LLSOCKET_CALL_C
#endif

/// @summary Abstract away Windows DLL import/export.
#if defined(_MSC_VER)
    #define  LLSOCKET_IMPORT    __declspec(dllimport)
    #define  LLSOCKET_EXPORT    __declspec(dllexport)
#else
    #define  LLSOCKET_IMPORT
    #define  LLSOCKET_EXPORT
#endif

/// @summary Define import/export based on whether we're being used as a DLL.
#if defined(LLSOCKET_SHARED)
    #ifdef  LLSOCKET_EXPORTS
    #define LLSOCKET_PUBLIC     LLSOCKET_EXPORT
    #else
    #define LLSOCKET_PUBLIC     LLSOCKET_IMPORT
    #endif
#else
    #define LLSOCKET_PUBLIC
#endif

/*///////////////////////
//   Namespace Begin   //
///////////////////////*/
namespace network {

/*//////////////////
//   Data Types   //
//////////////////*/
/// @summary Hide platform differences in the definition of a socket.
#if defined(_WIN32) || defined(_WIN64)
    typedef SOCKET            socket_t;
    typedef SOCKADDR_STORAGE  sockaddr_storage;
    #define INVALID_SOCKET_ID INVALID_SOCKET
#else
    typedef int               socket_t;
    #define INVALID_SOCKET_ID -1
#endif

/// @summary A function pointer type that can be passed into network::shutdown()
/// to process any data received after the socket is shutdown.
/// @param buffer A pointer to the start of the buffer of data to process.
/// @param buffer_size The number of bytes in @a buffer which contain data.
/// @param context Optional opaque data passed by the caller to shutdown().
typedef void (LLSOCKET_CALL_C *socket_flush_fn)(void *buffer, size_t buffer_size, void *context);

/*/////////////////
//   Functions   //
/////////////////*/
/// @summary Performs any system-specific initialization for the underlying
/// sockets library. This function should be called successfully once for each
/// process that will use sockets functionality.
/// @return true if initialization was successful.
LLSOCKET_PUBLIC bool startup(void);

/// @summary Performs any system-specific cleanup for the underlying sockets
/// library. This function should be called once for each process that will
/// use the sockets functionality.
LLSOCKET_PUBLIC void cleanup(void);

/// @summary Tests a return code from a sockets API call to determine whether
/// it indicates that an error occurred.
/// @param return_value The return value from the sockets API.
/// @return true if @a return_value indicates that an error occurred.
LLSOCKET_PUBLIC bool socket_error(int return_value);

/// @summary Checks a socket handle to determine whether it is valid.
/// @param sockfd The socket handle to check.
/// @return true if the specified socket handle represents a valid socket.
LLSOCKET_PUBLIC bool socket_valid(network::socket_t const &sockfd);

/// @summary Shuts down a socket in both directions and releases associated
/// resources. This function can be called to close a client or a server socket.
/// @param sockfd The socket handle to close.
LLSOCKET_PUBLIC void stream_close(network::socket_t const &sockfd);

/// @summary Creates a TCP streaming 'server' socket listening on the specified
/// port. The socket is created as a blocking socket. This process creates the
/// socket, binds it to the default interface on the specified port, and places
/// it into listen mode.
/// @param service_or_port Pointer to a NULL-terminated string specifying a
/// service name (ex. 'http') or a port number ('2122') on which the socket
/// will listen for incoming connections.
/// @param backlog The size of the socket backlog.
/// @param local_only Specify true to bind the server socket to the loopback
/// address, which only allows local clients to connect.
/// @param out_server_sockfd On return, the handle of the listening socket is
/// stored in this location. This value cannot be null.
/// @return true if the server socket was created successfully.
LLSOCKET_PUBLIC bool stream_listen(
    char const        *service_or_port,
    size_t             backlog,
    bool               local_only,
    network::socket_t *out_server_sockfd,
    int               *out_error = NULL);

/// @summary Accepts a single incoming connection on a server socket. If no
/// incoming connection is available to be processed, the calling thread is
/// blocked until a connection is available or and error occurs. If requested,
/// the client socket is placed into non-blocking mode.
/// @param server_sockfd The server socket that listens for incoming
/// connection attempts, as returned by the stream_listen() function.
/// @param non_blocking Specify true to place the socket into non-blocking
/// mode, so that the calling thread will not block during socket operations
/// such as stream_read() and stream_write().
/// @param out_client_sockfd On return, the handle of the socket that can be
/// used to send data to and receive data from the client.
/// @param out_client_info A pointer to a sockaddr_storage structure that is
/// filled with information about the connected client. This value is optional
/// and may be NULL.
/// @param out_client_info_size A pointer to a value that will be filled with
/// the size, in bytes, of the returned sockaddr_storage structure. This value
/// is optional and may be NULL.
/// @return true if the client socket connection was accepted successfully.
LLSOCKET_PUBLIC bool stream_accept(
    network::socket_t const &server_sockfd,
    bool                     non_blocking,
    network::socket_t       *out_client_sockfd,
    struct addrinfo         *out_client_info,
    size_t                  *out_client_info_size,
    int                     *out_error = NULL);

/// @summary Attempts to establish a connection to a listening server. If
/// requested and the connection attempt is successful, the socket is placed
/// into non-blocking mode.
/// @param host_or_address A NULL-terminated string specifying the host name
/// or IP address of the system to connect to, for example 'www.example.com'
/// or '192.168.0.19'. This value is required.
/// @param service_or_port A NULL-terminated string specifying a service name
/// such as 'http' or a port number such as '2122' used to determine the port
/// to connect to on the server.
/// @param non_blocking Specify true to place the socket into non-blocking
/// mode, so that the calling thread will not block during socket operations
/// such as read() and write().
/// @param out_remote_sockfd On return, this location is updated with a handle
/// to the socket that can be used to send data to and receive data from the server.
/// @return true if the connection attempt was successful.
LLSOCKET_PUBLIC bool stream_connect(
    char const        *host_or_address,
    char const        *service_or_port,
    bool               non_blocking,
    network::socket_t *out_remote_sockfd,
    int               *out_error = NULL);

/// @summary Attempts to read data from a socket into a caller-managed buffer.
/// Data is read until either the buffer is full, or there is no more data
/// remaining to read on the socket. If the socket has not previously been
/// placed into non-blocking I/O mode, the calling thread will block until data
/// is available to read.
/// @param sockfd The socket to read from.
/// @param buffer The caller-managed buffer into which any data read from the
/// socket will be stored.
/// @param buffer_size The total size of @a buffer, specified in bytes.
/// @param buffer_offset The current offset into @a buffer at which to start
/// writing data received on the socket.
/// @param out_disconnected On return, this value is set to non-zero if the
/// socket was disconnected. This value is required and cannot be NULL.
/// @return The number of bytes received on the socket and written into the
/// buffer. If no data was available, but the socket is still connected, this
/// value is zero and @a out_disconnected is false.
LLSOCKET_PUBLIC size_t stream_read(
    network::socket_t const &sockfd,
    void                    *buffer,
    size_t                   buffer_size,
    size_t                   buffer_offset,
    bool                    *out_disconnected,
    int                     *out_error = NULL);

/// @summary Reads data from a caller-managed buffer and writes it to the
/// socket. The write may be split into multiple calls as needed.
/// @param sockfd The socket to write to.
/// @param buffer The caller-managed buffer containing the data to send.
/// @param buffer_size The total size of @a buffer, specified in bytes.
/// @param buffer_offset The offset into @a buffer at which to begin reading.
/// @param amount_to_send The number of bytes to read from @a buffer.
/// @param out_disconnected On return, this value is set to non-zero if the
/// socket was disconnected. This value is required and cannot be NULL.
/// @return The number of bytes read from @a buffer and successfully written
/// to the socket. This value may be greater than zero, but less than the
/// @a amount_to_send if the socket was disconnected mid-process.
LLSOCKET_PUBLIC size_t stream_write(
    network::socket_t const &sockfd,
    void const              *buffer,
    size_t                   buffer_size,
    size_t                   buffer_offset,
    size_t                   amount_to_send,
    bool                    *out_disconnected,
    int                     *out_error = NULL);

/// @summary Gracefully shuts down a socket connection. Server sockets can
/// just be closed directly. Client sockets (whether created by stream_accept()
/// or stream_connect()) should use this function to gracefully close down the
/// connection. After this function returns, the socket has been completely closed.
/// @param sockfd The socket to shut down.
/// @param rxdata_callback An optional function pointer to a function that may
/// be called one or more times with data received on the connection after the
/// the socket has been shutdown. This value may be NULL.
/// @param rxdata_context A pointer to opaque data to be passed to the
/// @a rxdata_callback function. This value may be NULL.
LLSOCKET_PUBLIC void stream_shutdown(
    network::socket_t const  &sockfd,
    network::socket_flush_fn  rxdata_callback,
    void                     *rxdata_context);

/*/////////////////////
//   Namespace End   //
/////////////////////*/
}; /* end namespace network */

#endif /* LLSOCKET_HPP_INCLUDED */
