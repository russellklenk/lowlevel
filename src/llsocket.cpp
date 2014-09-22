/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements the runtime interface to the system sockets library; on
/// Windows the WinSock2 API is used while OSX/Linux uses the BSD sockets API.
/// @author Russell Klenk (contact@russellklenk.com)
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/
#include <string.h>
#include "llsocket.hpp"

/*/////////////////
//   Constants   //
/////////////////*/
// because having this statement everywhere is ugly.
#define LLSOCKET_SET_ERROR_RESULT(error_code) \
    if (out_error) *out_error  =  error_code

/// Define the timeout used (in microseconds) to wait for a socket to become
/// available if a read or write operation would block because buffers are
/// full (WSAENOBUFS/ENOBUFS/WSAEWOULDBLOCK/EAGAIN). If this time interval
/// elapses, the socket is shutdown. There are 1000000 microseconds per second;
/// the default value is five seconds.
#ifndef LLSOCKET_WAIT_TIMEOUT_USEC
    #define LLSOCKET_WAIT_TIMEOUT_USEC 5000000U
#endif /* !defined(LLSOCKET_WAIT_TIMEOUT_USEC) */

/// Define the maximum number of times the network::stream_read() or
/// network::stream_write() functions will retry receiving or sending of data
/// after recovering from a full buffer condition. If this many retries occur,
/// without successfully completing the operation, the socket is disconnected.
#ifndef LLSOCKET_MAX_RETRIES
    #define LLSOCKET_MAX_RETRIES       5U
#endif /* !defined(LLSOCKET_MAX_RETRIES) */

/*///////////////////////
//   Local Functions   //
///////////////////////*/
/// @summary Blocks the calling thread until a socket becomes available for
/// receiving data, or the specified timeout interval has elapsed.
/// @param sockfd The socket descriptor to wait on.
/// @param timeout_usec The timeout, in microseconds.
/// @return true if the socket is ready, or false if the operation timed out.
static bool wait_for_read(network::socket_t const &sockfd, uint64_t timeout_usec)
{
    const  uint64_t  usec_per_sec = 1000000;
    struct timeval   timeout      = {0};
    fd_set           fd_read;
    fd_set           fd_error;

    // build the timeout structure.
    timeout.tv_sec  = (long) (timeout_usec / usec_per_sec);
    timeout.tv_usec = (long) (timeout_usec % usec_per_sec);

    // build the file descriptor list we're polling.
    FD_ZERO(&fd_read);
    FD_ZERO(&fd_error);
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable:4127)
#endif /* _MSC_VER */
    FD_SET(sockfd, &fd_read);
    FD_SET(sockfd, &fd_error);
#ifdef _MSC_VER
#pragma warning (pop)
#endif /* _MSC_VER */

    // poll the client socket using select().
    int res  = select(FD_SETSIZE, &fd_read, NULL, &fd_error, &timeout);
    if (res == 1)
    {
        // the socket is now ready; we didn't time out.
        // reads to the socket should not block.
        return true;
    }
    if (res == 0)
    {
        // the wait timed out.
        return false;
    }
    // else, an error occurred during the wait.
    return false;
}

/// @summary Blocks the calling thread until a socket becomes available for
/// sending data, or the specified timeout interval has elapsed.
/// @param sockfd The socket descriptor to wait on.
/// @param timeout_usec The timeout, in microseconds.
/// @return true if the socket is ready, or false if the operation timed out.
static bool wait_for_write(network::socket_t const &sockfd, uint64_t timeout_usec)
{
    const  uint64_t  usec_per_sec = 1000000;
    struct timeval   timeout      = {0};
    fd_set           fd_write;
    fd_set           fd_error;

    // build the timeout structure.
    timeout.tv_sec  = (long) (timeout_usec / usec_per_sec);
    timeout.tv_usec = (long) (timeout_usec % usec_per_sec);

    // build the file descriptor list we're polling.
    FD_ZERO(&fd_write);
    FD_ZERO(&fd_error);
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable:4127)
#endif /* _MSC_VER */
    FD_SET(sockfd, &fd_write);
    FD_SET(sockfd, &fd_error);
#ifdef _MSC_VER
#pragma warning (pop)
#endif /* _MSC_VER */

    // poll the client socket using select().
    int res  = select(FD_SETSIZE, NULL, &fd_write, &fd_error, &timeout);
    if (res == 1)
    {
        // the socket is now ready; we didn't time out.
        // writes to the socket should not block.
        return true;
    }
    if (res == 0)
    {
        // the wait timed out.
        return false;
    }
    // else, an error occurred during the wait.
    return false;
}

/*////////////////////////
//   Public Functions   //
////////////////////////*/
bool network::startup(void)
{
#if defined(_WIN32) || defined(_WIN64)
    WORD    version = MAKEWORD(2, 2);
    WSAData wsaData = {0};
    if (WSAStartup(version, &wsaData) != 0)
    {
        // the winsock library could not be initialized.
        return false;
    }
#endif
    return true;
}

void network::cleanup(void)
{
#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif
}

bool network::socket_error(int return_value)
{
#if defined(_WIN32) || defined(_WIN64)
    return (return_value == SOCKET_ERROR);
#else
    return (return_value  < 0);
#endif
}

bool network::socket_valid(network::socket_t const &sockfd)
{
    // @note: INVALID_SOCKET_ID = INVALID_SOCKET on Windows or -1 on everything else.
    return (sockfd != INVALID_SOCKET_ID);
}

void network::stream_close(network::socket_t const &sockfd)
{
#if defined(_WIN32) || defined(_WIN64)
    closesocket(sockfd);
#else
    close(sockfd);
#endif
}

bool network::stream_listen(
    char const        *service_or_port,
    size_t             backlog,
    bool               local_only,
    network::socket_t *out_server_sockfd,
    int               *out_error /* = NULL */)
{
    struct addrinfo    hints = {0};
    struct addrinfo   *info  = NULL;
    struct addrinfo   *iter  = NULL;
    network::socket_t  sock  = INVALID_SOCKET_ID;
    int                yes   = 1;
    int                res   = 0;

    LLSOCKET_SET_ERROR_RESULT(0);

    if (service_or_port == NULL || out_server_sockfd == NULL)
    {
        // invalid parameter. fail immediately.
        if (out_server_sockfd != NULL) *out_server_sockfd = INVALID_SOCKET_ID;
        return false;
    }

    // ask the system to fill out the addrinfo structure we'll use to set up
    // the socket. this method works with both IPv4 and IPv6.
#if   defined(LLSOCKET_IPV4)
    hints.ai_family   = AF_INET;
#elif defined(LLSOCKET_IPV6)
    hints.ai_family   = AF_INET6;
#else
    hints.ai_family   = AF_UNSPEC;   // AF_INET, AF_INET6 or AF_UNSPEC
#endif
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags    = local_only ? 0 : AI_PASSIVE;
    res = getaddrinfo(NULL, service_or_port, &hints, &info);
    if (res != 0)
    {
        // getaddrinfo failed. inspect res to see what the problem was.
        *out_server_sockfd = INVALID_SOCKET_ID;
        LLSOCKET_SET_ERROR_RESULT(res);
        return false;
    }

    // iterate through the returned interface information and attempt to bind
    // a socket to the interface. stop as soon as we succeed.
    for (iter = info; iter != NULL; iter = iter->ai_next)
    {
        sock  = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol);
        if (sock == INVALID_SOCKET_ID)  continue;
        // prevent an 'address already in use' error message which could
        // occur if a socket connected to this port previously hasn't finished
        // being shut down by the operating system yet. we don't want to
        // wait for the operating system timeout to occur.
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(yes));
        // bind to the specified address/port.
        res = bind(sock, iter->ai_addr, int(iter->ai_addrlen));
        if (network::socket_error(res))
        {
            network::stream_close(sock);
            continue;
        }
        // if we've gone this far, we're done.
        break;
    }

    // were we able to bind to the interface?
    if (iter == NULL)
    {
        freeaddrinfo(info);
        *out_server_sockfd = INVALID_SOCKET_ID;
        LLSOCKET_SET_ERROR_RESULT(res);
        return false;
    }

    // we no longer need the addrinfo list, so free it.
    freeaddrinfo(info); info = NULL;

    // start the socket in listen mode.
    res = listen(sock, (int) backlog);
    if (network::socket_error(res))
    {
        network::stream_close(sock);
        *out_server_sockfd = INVALID_SOCKET_ID;
        LLSOCKET_SET_ERROR_RESULT(res);
        return false;
    }

    // we've completed socket setup successfully.
    *out_server_sockfd = sock;
    return true;
}

bool network::stream_accept(
    network::socket_t const &server_sockfd,
    bool                     non_blocking,
    network::socket_t       *out_client_sockfd,
    struct addrinfo         *out_client_info,
    size_t                  *out_client_info_size,
    int                     *out_error /*  = NULL */)
{
    socklen_t         client_size = sizeof(sockaddr_storage);
    sockaddr_storage  client_addr = {0};
    network::socket_t sock        = INVALID_SOCKET_ID;

    LLSOCKET_SET_ERROR_RESULT(0);

    if (server_sockfd == INVALID_SOCKET_ID || out_client_sockfd == NULL)
    {
        // invalid parameter. fail immediately.
        if (out_client_sockfd    != NULL) *out_client_sockfd    = INVALID_SOCKET_ID;
        if (out_client_info_size != NULL) *out_client_info_size = 0;
        return false;
    }

    // accept() will block until a connection is ready or an error occurs.
    sock = accept(server_sockfd, (struct sockaddr*)&client_addr, &client_size);
    if (sock == INVALID_SOCKET_ID)
    {
        *out_client_sockfd = INVALID_SOCKET_ID;
        if (out_client_info_size != NULL) *out_client_info_size = 0;
        return false;
    }

#if defined(_WIN32) || defined(_WIN64)
    if (non_blocking)
    {
        // place sock into non-blocking mode.
        u_long nbio_mode = 1;
        ioctlsocket(sock, FIONBIO, &nbio_mode);
    }
#else
    if (non_blocking)
    {
        // place sock into non-blocking mode.
        fcntl(sock, F_SETFL, O_NONBLOCK);
    }
#endif

    // we are done; store information for the caller.
    *out_client_sockfd   = sock;
    if (out_client_info != NULL)
    {
        // copy client data for the caller.
        memcpy(out_client_info, &client_addr, client_size);
    }
    if (out_client_info_size != NULL) *out_client_info_size = client_size;
    return true;
}

bool network::stream_connect(
    char const        *host_or_address,
    char const        *service_or_port,
    bool               non_blocking,
    network::socket_t *out_remote_sockfd,
    int               *out_error /* = NULL */)
{
    struct addrinfo    hints = {0};
    struct addrinfo   *info  = NULL;
    struct addrinfo   *iter  = NULL;
    network::socket_t  sock  = INVALID_SOCKET_ID;
    int                res   =  0;

    LLSOCKET_SET_ERROR_RESULT(0);

    if (host_or_address   == NULL ||
        service_or_port   == NULL ||
        out_remote_sockfd == NULL)
    {
        // invalid parameter. fail immediately.
        if (out_remote_sockfd != NULL) *out_remote_sockfd = INVALID_SOCKET_ID;
        return false;
    }

    // ask the system to fill out the addrinfo structure we'll use to set up
    // the socket. this method works with both IPv4 and IPv6.
#if   defined(LLSOCKET_IPV4)
    hints.ai_family   = AF_INET;
#elif defined(LLSOCKET_IPV6)
    hints.ai_family   = AF_INET6;
#else
    hints.ai_family   = AF_UNSPEC;   // v4 (AF_INET), v6 (AF_INET6), AF_UNSPEC
#endif
    hints.ai_socktype = SOCK_STREAM; // TCP
    res = getaddrinfo(host_or_address, service_or_port, &hints, &info);
    if (res != 0)
    {
        // getaddrinfo failed. inspect res to see what the problem was.
        *out_remote_sockfd = INVALID_SOCKET_ID;
        LLSOCKET_SET_ERROR_RESULT(res);
        return false;
    }

    // iterate through the returned interfce information and attempt to
    // connect. stop as soon as we succeed.
    for (iter = info; iter != NULL; iter = iter->ai_next)
    {
        sock  = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol);
        if (INVALID_SOCKET_ID == sock) continue;
        res   = connect(sock, iter->ai_addr, int(iter->ai_addrlen));
        if (network::socket_error(res))
        {
            network::stream_close(sock);
            continue;
        }
        // we've successfully established a connection.
        break;
    }

    // were we able to connect to the server?
    if (iter == NULL)
    {
        freeaddrinfo(info);
        *out_remote_sockfd = INVALID_SOCKET_ID;
        LLSOCKET_SET_ERROR_RESULT(res);
        return false;
    }

    // we no longer need the addrinfo list, so free it.
    freeaddrinfo(info); info = NULL;

#if defined(_WIN32) || defined(_WIN64)
    if (non_blocking)
    {
        // place sock into non-blocking mode.
        u_long nbio_mode = 1;
        ioctlsocket(sock, FIONBIO, &nbio_mode);
    }
#else
    if (non_blocking)
    {
        // place sock into non-blocking mode.
        fcntl(sock, F_SETFL, O_NONBLOCK);
    }
#endif

    // we've completed socket connection successfully.
    *out_remote_sockfd = sock;
    return true;
}

size_t network::stream_read(
    network::socket_t const &sockfd,
    void                    *buffer,
    size_t                   buffer_size,
    size_t                   buffer_offset,
    bool                    *out_disconnected,
    int                     *out_error /* = NULL */)
{
    char *buf = (char*) buffer      + buffer_offset;
    int   nba = (int)  (buffer_size - buffer_offset);
    int   res = -1;

    LLSOCKET_SET_ERROR_RESULT(0);

    if (buffer           == NULL   ||
        buffer_size      == 0      ||
        out_disconnected == NULL   ||
        buffer_offset    >= buffer_size)
    {
        // invalid parameter. return immediately.
        if (out_disconnected != NULL) *out_disconnected = false;
        return 0;
    }

    // ideally this loop only executes once, but if the OS buffers fill up
    // we may need to re-try the receive operation multiple times.
    size_t retry_count = 0;
    bool   try_recv    = true;
    while (try_recv)
    {
        if (retry_count >= LLSOCKET_MAX_RETRIES)
        {
            // this socket has exceeded its retry count. disconnect it.
            network::stream_shutdown(sockfd, NULL, NULL);
            *out_disconnected = true;
            return 0;
        }

        // assume the receive operation will succeed:
        try_recv = false;

        // read as much data as possible from the socket:
        res = recv(sockfd, buf, nba, 0);
        if  (!network::socket_error(res))
        {
            if (res != 0)
            {
                *out_disconnected = false;
                return (size_t) res;
            }
            else
            {
                // the socket was shutdown gracefully.
                network::stream_shutdown(sockfd, NULL, NULL);
                *out_disconnected = true;
                return 0;
            }
        }

#if defined(_WIN32) || defined(_WIN64)
        // an error occurred. try to determine whether we are still connected.
        int     err = WSAGetLastError();
        switch (err)
        {
            case WSAEWOULDBLOCK:
                {
                    // no data is available to read on the socket.
                    // this is not actually an error.
                    *out_disconnected = false;
                }
                return 0;

            case WSAENOBUFS:     // ENOBUFS
                {
                    // insufficient resources available in the system to
                    // perform the operation. we'll wait (in select()) for
                    // a bit to see if the socket becomes readable again;
                    // otherwise, the socket is shutdown.
                    if (wait_for_read(sockfd, LLSOCKET_WAIT_TIMEOUT_USEC))
                    {
                        // the socket is available again. retry.
                        try_recv = true;
                        retry_count++;
                        continue; // back to the loop start
                    }
                    else
                    {
                        // the wait timed out. disconnect the socket.
                        network::stream_shutdown(sockfd, NULL, NULL);
                        LLSOCKET_SET_ERROR_RESULT(err);
                        *out_disconnected = true;
                    }
                }
                return 0;

            case WSANOTINITIALISED:
            case WSAENETDOWN:
            case WSAENOTCONN:
            case WSAENETRESET:
            case WSAENOTSOCK:
            case WSAESHUTDOWN:
            case WSAECONNABORTED:
            case WSAETIMEDOUT:
            case WSAECONNRESET:
                {
                    // we were either never connected, or the connection has
                    // somehow been terminated in a non-graceful way.
                    network::stream_shutdown(sockfd, NULL, NULL);
                    LLSOCKET_SET_ERROR_RESULT(err);
                    *out_disconnected = true;
                }
                return 0;

            default:
                {
                    // an error occurred; try again.
                    try_recv = true;
                    retry_count++;
                }
                break;
        }
#else
        // an error occurred. try to determine whether we are still connected.
        int     err = errno;
        switch (err)
        {
            case EWOULDBLOCK:
                {
                    // no data is available to read on the socket.
                    // this is not actually an error.
                    *out_disconnected = false;
                }
                return 0;

            case ENOBUFS:
                {
                    // insufficient resources available in the system to
                    // perform the operation. we'll wait (in select()) for
                    // a bit to see if the socket becomes readable again;
                    // otherwise, the socket is shutdown.
                    if (wait_for_read(sockfd, LLSOCKET_WAIT_TIMEOUT_USEC))
                    {
                        // the socket is available again. retry.
                        try_recv = true;
                        retry_count++;
                        continue; // back to the loop start
                    }
                    else
                    {
                        // the wait timed out. disconnect the socket.
                        network::stream_shutdown(sockfd, NULL, NULL);
                        LLSOCKET_SET_ERROR_RESULT(err);
                        *out_disconnected = true;
                    }
                }
                return 0;

            case EBADF:
            case ECONNRESET:
            case ENOTCONN:
            case ENOTSOCK:
            case ETIMEDOUT:
                {
                    // we were either never connected, or the connection has
                    // somehow been terminated in a non-graceful way.
                    network::stream_shutdown(sockfd, NULL, NULL);
                    LLSOCKET_SET_ERROR_RESULT(err);
                    *out_disconnected = true;
                }
                return 0;

            default:
                {
                    // an error occurred; try again.
                    try_recv = true;
                    retry_count++;
                }
                break;
        }
#endif
    }
    // no data was received.
    return 0;
}

size_t network::stream_write(
    network::socket_t const &sockfd,
    void const              *buffer,
    size_t                   buffer_size,
    size_t                   buffer_offset,
    size_t                   amount_to_send,
    bool                    *out_disconnected,
    int                     *out_error /* = NULL */)
{
    char const *buf = (char*) buffer + buffer_offset;

    LLSOCKET_SET_ERROR_RESULT(0);

    if (buffer           == NULL     ||
        buffer_size      == 0        ||
        out_disconnected == NULL     ||
        buffer_offset  > buffer_size ||
        amount_to_send > buffer_size - buffer_offset)
    {
        // no data or invalid parameter. return immediately.
        if (out_disconnected != NULL) *out_disconnected = false;
        return 0;
    }

    // by default, we have not disconnected. if we detect a disconnection
    // while sending, we will set this value to true.
    *out_disconnected    = false;

    // enter a loop to ensure that all of the data gets sent.
    // we may have to issue multiple send calls to the socket.
    size_t retry_count   = 0;
    size_t bytes_sent    = 0;
    size_t bytes_total   = amount_to_send;
    while (bytes_sent    < bytes_total)
    {
        if (retry_count >= LLSOCKET_MAX_RETRIES)
        {
            // this socket has exceeded its retry count. disconnect it.
            network::stream_shutdown(sockfd, NULL, NULL);
            *out_disconnected = true;
            return 0;
        }

        // attempt to send as much data as we can write to the socket.
        int n_to_send   = (int) bytes_total - (int) bytes_sent;
        int n_sent      = send(sockfd, buf, n_to_send, 0);
        if (!network::socket_error(n_sent) && n_sent > 0)
        {
            // @note: do NOT reset retry_count to zero in this case.
            // under Windows at least, the socket will become available
            // again, but immediately fail. awesomeness.
            bytes_sent  += n_sent;
            buf         += n_sent;
            continue;
        }

#if defined(_WIN32) || defined(_WIN64)
        // an error occurred. determine whether we are still connected.
        int     err = WSAGetLastError();
        switch (err)
        {
            case WSANOTINITIALISED:
            case WSAENETDOWN:
            case WSAENETRESET:
            case WSAENOTCONN:
            case WSAENOTSOCK:
            case WSAESHUTDOWN:
            case WSAEHOSTUNREACH:
            case WSAEINVAL:
            case WSAECONNABORTED:
            case WSAECONNRESET:
            case WSAETIMEDOUT:
                {
                    // something has gone wrong with the connection.
                    network::stream_shutdown(sockfd, NULL, NULL);
                    LLSOCKET_SET_ERROR_RESULT(err);
                    *out_disconnected = true;
                }
                return bytes_sent;

            case WSAEACCES:
                {
                    // attempted to send to a broadcast address without
                    // setting the appropriate socket options.
                    network::stream_shutdown(sockfd, NULL, NULL);
                    LLSOCKET_SET_ERROR_RESULT(err);
                    *out_disconnected = true;
                }
                return bytes_sent;

            case WSAENOBUFS:     // ENOBUFS
            case WSAEWOULDBLOCK: // EAGAIN
                {
                    // the resource is temporarily unavailable, probably
                    // because the socket is being flooded with data. we
                    // wait until the socket becomes available again, or
                    // our timeout interval elapses. if the socket becomes
                    // available, we continue sending; if the select times
                    // out, we forcibly disconnect the client.
                    if (wait_for_write(sockfd, LLSOCKET_WAIT_TIMEOUT_USEC))
                    {
                        // the socket is available again. retry.
                        retry_count++;
                        break;
                    }
                    else
                    {
                        // the wait timed out. disconnect the socket.
                        network::stream_shutdown(sockfd, NULL, NULL);
                        LLSOCKET_SET_ERROR_RESULT(err);
                        *out_disconnected = true;
                    }
                }
                return bytes_sent;

            default:
                {
                    // unknown error; try again.
                    LLSOCKET_SET_ERROR_RESULT(err);
                    retry_count++;
                }
                break;
        }
#else
        // an error occurred. determine whether we are still connected.
        int     err = errno;
        switch (err)
        {
            case EBADF:
            case ECONNRESET:
            case ENOTCONN:
            case ENOTSOCK:
            case ETIMEDOUT:
            case EHOSTUNREACH:
            case ENETDOWN:
            case ENETUNREACH:
            case EPIPE:
                {
                    // something has gone wrong with the connection.
                    network::stream_shutdown(sockfd, NULL, NULL);
                    LLSOCKET_SET_ERROR_RESULT(err);
                    *out_disconnected = true;
                }
                return bytes_sent;

            case EACCES:
                {
                    // attempted to send to a broadcast address without
                    // setting the appropriate socket options.
                    network::stream_shutdown(sockfd, NULL, NULL);
                    LLSOCKET_SET_ERROR_RESULT(err);
                    *out_disconnected = true;
                }
                return bytes_sent;

            case ENOBUFS:
            case EAGAIN:
                {
                    // the resource is temporarily unavailable, probably
                    // because the socket is being flooded with data. we
                    // wait until the socket becomes available again, or
                    // our timeout interval elapses. if the socket becomes
                    // available, we continue sending; if the select times
                    // out, we forcibly disconnect the client.
                    if (wait_for_write(sockfd, LLSOCKET_WAIT_TIMEOUT_USEC))
                    {
                        // the socket is available again. retry.
                        retry_count++;
                        break;
                    }
                    else
                    {
                        // the wait timed out. disconnect the socket.
                        network::stream_shutdown(sockfd, NULL, NULL);
                        LLSOCKET_SET_ERROR_RESULT(err);
                        *out_disconnected = true;
                    }
                }
                return bytes_sent;

            default:
                {
                    // unknown error; try again.
                    LLSOCKET_SET_ERROR_RESULT(err);
                    retry_count++;
                }
                break;
        }
#endif
    }
    return bytes_sent;
}

void network::stream_shutdown(
    network::socket_t const  &sockfd,
    network::socket_flush_fn  rxdata_callback,
    void                     *rxdata_context)
{
    size_t const SDBUF_SIZE         = 4096;
    uint8_t      buffer[SDBUF_SIZE] = {0};
    size_t       rx_size            =  0;
    bool         disconn            = false;
#if defined(_WIN32) || defined(_WIN64)
    int          res                = SOCKET_ERROR;
#else
    int          res                = -1;
#endif

    if (sockfd == INVALID_SOCKET_ID)
    {
        // invalid socket. complete immediately.
        return;
    }

    // shutdown the socket in the send direction. we are saying we won't
    // be sending any more data from this side of the connection.
#if defined(_WIN32) || defined(_WIN64)
    res = shutdown(sockfd, SD_SEND);
#else
    res = shutdown(sockfd, SHUT_WR);
#endif
    if (network::socket_error(res))
    {
        // just close the socket immediately.
        network::stream_close(sockfd);
        return;
    }

    // continue to receive data until the other end disconnects.
    while (rxdata_callback != NULL && !disconn)
    {
        rx_size = network::stream_read(sockfd, buffer, SDBUF_SIZE, 0, &disconn);
        if (rx_size > 0 && rxdata_callback != NULL)
        {
            // let the caller process the data.
            rxdata_callback(buffer, rx_size, rxdata_context);
        }
    }

    // finally, close the socket down completely.
    network::stream_close(sockfd);
}
