/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: simo <simo@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/20 23:27:14 by msitni1337        #+#    #+#             */
/*   Updated: 2025/02/24 13:10:22 by simo             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#ifdef MAC_BUILD
#include <bsd/string.h>
#else
#include <cstring>
#endif
#include "Config.hpp"
#include "Exceptions.hpp"
#include "IOEventListener.hpp"
#include "IOMultiplexer.hpp"
#include "Response.hpp"
#include "ServerClient.hpp"
#include "ServerUtils.hpp"
#include <cassert>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <queue>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

class Server : public AIOEventListener
{

private:
    std::vector<ServerClient*> _clients_pool;
    std::vector<ServerConfig>  _config;
    bool                       _is_started;
    std::vector<sockaddr_in>   _listen_addrs;
    std::vector<int>           _listen_socket_fds;
    epoll_event                _listen_socket_ev;

public:
    ~Server();

private:
    Server(const std::vector<ServerConfig>& config);
    Server(const Server& server);
    Server& operator=(const Server& server);

public:
    void Start();

    /* Getters & Setters */
public:
    bool                             is_started() const;
    const std::vector<ServerConfig>& GetConfig() const;
    const std::vector<int>&          GetListenSockets() const;

    /* Interface */
public:
    virtual void ConsumeEvent(const epoll_event ev);
    void         Terminate();

    /* Private Methods */
private:
    void listen_on_addr(const sockaddr_in& _listen_addr);
    void AcceptNewPeerOnSocket(int socket_fd);

public:
    static Server& GetInstance();
};
