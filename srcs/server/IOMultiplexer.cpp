/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   IOMultiplexer.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msitni <msitni@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/22 17:30:28 by msitni1337        #+#    #+#             */
/*   Updated: 2025/02/24 04:27:35 by msitni           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "IOMultiplexer.hpp"
#include <cstdlib>

int webserv_unix_signal = 0; // Global variable signal

IOMultiplexer::IOMultiplexer() : _is_started(false)
{
    _epoll_fd = epoll_create(__INT32_MAX__);
    if (_epoll_fd == -1)
        throw IOMultiplexerException("epoll_create(): failed.");
}
IOMultiplexer& IOMultiplexer::GetInstance()
{
    static IOMultiplexer instance;
    return instance;
}
IOMultiplexer::~IOMultiplexer()
{
    Terminate();
    close(_epoll_fd);
}
const int& IOMultiplexer::GetEpollFd() const
{
    return _epoll_fd;
}
void IOMultiplexer::AddEvent(AIOEventListener* listener, epoll_event ev)
{
    std::map<int, AIOEventListener*>::iterator it = _listeners.find(ev.data.fd);
    if (it != _listeners.end())
        throw IOMultiplexerException("AddEvent() : Event listener already added.");
    _listeners.insert(std::pair<int, AIOEventListener*>(ev.data.fd, listener));
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
        throw IOMultiplexerException("epoll_ctl() failed.");
}
void IOMultiplexer::RemoveEvent(epoll_event ev, int fd)
{
    std::map<int, AIOEventListener*>::iterator it = _listeners.find(fd);
    if (it == _listeners.end())
        throw IOMultiplexerException("RemoveEvent() : Event listener not found.");
    ev.data.fd = fd;
    _listeners.erase(fd);
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, &ev) == -1)
        throw IOMultiplexerException("epoll_ctl() failed.");
}
void IOMultiplexer::StartEventLoop()
{
    if (_is_started)
        throw IOMultiplexerException("Events loop already started.");
    if (!_listeners.size())
    {
        std::cout << "No Virtual Server is running, program will quit." << std::endl;
        return;
    }
    _is_started = true;
    std::cout << ">>>> IOMultiplexer loop started." << std::endl;
    while (_is_started)
    {
        int events_count = epoll_wait(_epoll_fd, _events, EPOLL_EVENTS_MAX, -1);
        if (events_count == -1)
        {
            if (webserv_unix_signal == SIGINT)
                break;
            Terminate();
            throw IOMultiplexerException("epoll_wait(): failed.");
        }
        for (int i = 0; i < events_count; i++)
        {
            std::map<int, AIOEventListener*>::iterator it = _listeners.find(_events[i].data.fd);
            if (it == _listeners.end())
                throw IOMultiplexerException(
                    "fd not found in [std::map<int, AIOEventListener*> _listeners] member."
                );
            it->second->ConsumeEvent(_events[i]);
        }
        if (webserv_unix_signal == SIGINT)
            break;
    }
    Terminate();
}
void IOMultiplexer::Terminate()
{
    if (_is_started == false)
        return;
    _is_started                                           = false;
    std::map<int, AIOEventListener*>::reverse_iterator it = _listeners.rbegin();
    for (; it != _listeners.rend(); it = _listeners.rbegin())
        it->second->Terminate();
}