/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerClient.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msitni <msitni@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/22 11:55:35 by msitni            #+#    #+#             */
/*   Updated: 2025/02/24 04:32:01 by msitni           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ServerClient.hpp"
#include "Server.hpp"

ServerClient::ServerClient(const int& client_socket_fd, const int& address_socket_fd)
    : _is_started(false), _client_socket_fd(client_socket_fd),
      _address_socket_fd(address_socket_fd), _request(address_socket_fd)
{
}

ServerClient::ServerClient(const ServerClient& client)
    : AIOEventListener(client), _request(client._address_socket_fd)
{
    *this = client;
}

ServerClient& ServerClient::operator=(const ServerClient& client)
{
    if (this == &client)
        return *this;
    _is_started        = client._is_started;
    _client_socket_fd  = client._client_socket_fd;
    _address_socket_fd = client._address_socket_fd;
    _epoll_ev          = client._epoll_ev;
    _request           = client._request;
    _responses_queue   = client._responses_queue;
    _cgi_responses     = client._cgi_responses;
    return *this;
}

ServerClient::~ServerClient() {}

void ServerClient::BindToClientSocket()
{
    assert(_is_started == false);
    _epoll_ev.data.fd = _client_socket_fd;
    _epoll_ev.events  = EPOLLIN | EPOLLOUT;
    try
    {
        IOMultiplexer::GetInstance().AddEvent(this, _epoll_ev);
        std::cout << "<<<< New peer accepted on fd " << _client_socket_fd << ".\n";
        _is_started = true;
    }
    catch (const std::exception& e)
    {
        close(_client_socket_fd);
        std::cerr << "_IOmltplx->AddEvent() failed to add new peer fd: " << _client_socket_fd
                  << "Connection will terminate.\nReason: " << e.what() << std::endl;
    }
    _request = Request(_address_socket_fd);
}
void ServerClient::ConsumeEvent(const epoll_event ev)
{
    // CGI pipe I/O:
    std::map<int, ResponseCGI*>::iterator cgi_it = _cgi_responses.find(ev.data.fd);
    if (cgi_it != _cgi_responses.end())
    {
        HandleCGIEPOLLIN(ev, cgi_it->second);
    }
    else
    {
        // Client Requests:
        if (ev.events & EPOLLIN)
            HandleEPOLLIN();
        if (_responses_queue.size() && ev.events & EPOLLOUT)
            HandleEPOLLOUT();
    }
    // Checking CGI timeout.
    time_t now = time(0);
    cgi_it     = _cgi_responses.begin();
    for (; cgi_it != _cgi_responses.end();)
    {
        if (now - cgi_it->second->GetStartTime() > CGI_TIMEOUT)
        {
            kill(cgi_it->second->GetCGIPID(), SIGKILL);
            epoll_event ev;
            ev.data.ptr = this;
            ev.events   = EPOLLIN | EPOLLHUP;
            IOMultiplexer::GetInstance().RemoveEvent(ev, cgi_it->first);
            close(cgi_it->first);
            SendErrorResponse(HttpStatus(STATUS_GATEWAY_TIMEOUT), cgi_it->second);
            _cgi_responses.erase(cgi_it);
            cgi_it = _cgi_responses.begin();
        }
        else
        {
            cgi_it++;
        }
    }
    if (_request.getIsReceiving())
    {
        if (now - _request.getStartTime() > REQ_TIMEOUT)
        {
            _request.clear();
            Response* response = new Response();
            SendErrorResponse(HttpStatus(STATUS_REQUEST_TIMEOUT), response);
        }
    }
}
void ServerClient::HandleEPOLLIN()
{
    std::vector<uint8_t> buffer(RECV_CHUNK, 0);
    ssize_t              bytes = recv(_client_socket_fd, buffer.data(), RECV_CHUNK, MSG_DONTWAIT);
    if (bytes < 0)
    {
        std::cerr << "recv() failed. for client fd: " << _client_socket_fd << '\n';
        std::cerr << "Client will be kicked out." << std::endl;
        return Terminate();
    }
    if (bytes == 0)
        return Terminate();
    if ((size_t)bytes < READ_CHUNK)
        buffer.erase(buffer.begin() + bytes, buffer.end());
    ReceiveRequest(buffer);
}
void ServerClient::HandleEPOLLOUT()
{
fetch_next_response:
    Response* response = _responses_queue.front();
    if (response->GetResponseBuffCount() == 0)
    {
        delete response;
        _responses_queue.pop();
        if (_responses_queue.size())
            goto fetch_next_response;
        else
            return;
    }
    const uint8_t* buff;
    try
    {
        buff = response->GetResponseBuff();
    }
    catch (const std::exception& e)
    {
        assert(!"NOT IMPLEMENTED");
        std::cerr << e.what() << '\n';
        return;
    }
    size_t bytes_to_send = response->GetResponseBuffCount();
    if (bytes_to_send > SEND_CHUNK)
        bytes_to_send = SEND_CHUNK;
    ssize_t bytes_sent = send(_client_socket_fd, buff, bytes_to_send, MSG_DONTWAIT);
    if (bytes_sent < 0)
    {
        std::cerr << "send() failed. for client fd: " << _client_socket_fd << '\n';
        std::cerr << "Client will be kicked out." << std::endl;
        return Terminate();
    }
    if ((size_t)bytes_sent != bytes_to_send)
        std::cerr << "Tried to send " << bytes_to_send << " but send() only sent " << bytes_sent
                  << "\nRemainder data will be sent on next epoll event call." << std::endl;
    response->IncrementResponseBuffBytesSent(bytes_sent);
    if (response->GetResponseBuffCount() == 0)
    {
        delete response;
        _responses_queue.pop();
    }
}
void ServerClient::QueueCGIResponse(int output_pipe_fd, ResponseCGI* response)
{
    epoll_event ev;
    ev.events  = EPOLLIN | EPOLLHUP;
    ev.data.fd = output_pipe_fd;
    IOMultiplexer::GetInstance().AddEvent(this, ev);
    _cgi_responses.insert(std::pair<int, ResponseCGI*>(output_pipe_fd, response));
    std::cout << ">>>> IOMultiplexer is listening on CGI pipe fd: " << output_pipe_fd
              << " for client fd: " << _client_socket_fd << std::endl;
}
void ServerClient::HandleCGIEPOLLIN(const epoll_event& ev, Response* response)
{
    std::vector<uint8_t> buff(READ_CHUNK, 0);
    ssize_t              bytes = read(ev.data.fd, buff.data(), READ_CHUNK);
    if (bytes < 0)
    {
        std::cerr << "HandleCGIEPOLLIN(): read() failed for file script: "
                  << response->GetRequestFilePath() << std::endl;
        return SendErrorResponse(HttpStatus(STATUS_INTERNAL_SERVER_ERROR), response);
    }
    if (bytes == 0)
    {
        epoll_event event = ev;
        event.data.ptr    = this;
        _cgi_responses.erase(ev.data.fd);
        try
        {
            IOMultiplexer::GetInstance().RemoveEvent(event, ev.data.fd);
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        close(ev.data.fd);
        response->FinishResponse();
        _responses_queue.push(response);
        return;
    }
    if ((size_t)bytes < READ_CHUNK)
        buff.erase(buff.begin() + bytes, buff.end());
    response->AppendToResponseBuff(buff);
}
void ServerClient::Terminate()
{
    if (_is_started == false)
        throw ServerClientException("ServerClient::Terminate(): client not started.");
    for (; _responses_queue.size(); _responses_queue.pop())
    {
        close(_responses_queue.front()->GetRequestFileFd());
        close(_responses_queue.front()->GetRequest().getBodyFd());
        delete _responses_queue.front();
    }
    std::map<int, ResponseCGI*>::iterator cgi_it = _cgi_responses.begin();
    for (; cgi_it != _cgi_responses.end(); cgi_it++)
    {
        try
        {
            epoll_event ev;
            ev.events   = EPOLLIN | EPOLLHUP;
            ev.data.ptr = this;
            IOMultiplexer::GetInstance().RemoveEvent(ev, cgi_it->first);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Exception caught while terminating client fd: " << _client_socket_fd
                      << std::endl;
            std::cerr << "Reason: " << e.what() << std::endl;
        }
        close(cgi_it->first);
        kill(cgi_it->second->GetCGIPID(), SIGKILL);
        delete cgi_it->second;
    }
    _cgi_responses.clear();
    try
    {
        IOMultiplexer::GetInstance().RemoveEvent(_epoll_ev, _client_socket_fd);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception caught while terminating client fd: " << _client_socket_fd
                  << std::endl;
        std::cerr << "Reason: " << e.what() << std::endl;
    }
    close(_client_socket_fd);
    _is_started = false;
    std::cout << ">>>> Client fd: " << _client_socket_fd << " disconnected." << std::endl;
}

void ServerClient::ReceiveRequest(const std::vector<uint8_t>& buff)
{
    _request += buff;

    if (_request.isCompleted())
    {
        HttpStatus status = _request.getStatus();
        if (status.code != STATUS_OK)
        {
            Response* response = new Response();
            // response->SetClientSocketFd(_client_socket_fd);
            // response->SetVirtualServer(&_server->GetConfig().front());
            _request.clear();
            return SendErrorResponse(status, response);
        }
        std::cerr << "<<<< Client fd: " << _client_socket_fd
                  << " Requested uri: " << _request.getMethod() << " " << _request.getUri()
                  << " | Request status after parsing: " << status.message
                  << " is chunked: " << _request.isChunked() << std::endl;
        Response* response = new Response(_request);
        // response->SetClientSocketFd(_client_socket_fd);
        _request.clear();
        try
        {
            ProcessRequest(response);
        }
        catch (const std::exception& e)
        {
            SendErrorResponse(HttpStatus(STATUS_INTERNAL_SERVER_ERROR), response);
        }
    }
}

void ServerClient::ProcessRequest(Response* response)
{
    LocationIterator file_location = response->GetRequest().getRequestFileLocation();
    if (file_location == response->GetRequest().getRequestVirtualServer()->locations.end())
        return SendErrorResponse(HttpStatus(STATUS_NOT_FOUND), response);
    if (file_location->redirect)
    {
        std::string code_message = utils::to_string(file_location->redirect_code);
        response->SetStatusHeaders(code_message.c_str());
        ResponseHeader location_header;
        location_header.name  = "Location";
        location_header.value = file_location->redirect_path;
        response->AppendHeader(location_header);
        response->FinishResponse();
        _responses_queue.push(response);
        return;
    }
    HttpStatus check = CheckRequest(response);
    if (check.code == STATUS_HTTP_INTERNAL_IMPLEM_AUTO_INDEX)
        return;
    if (check.code != STATUS_OK)
        return SendErrorResponse(check, response);
    const std::string& method = response->GetRequest().getMethod();
    // HTTP CGI response:
    if (file_location->cgi_path != "")
    {
        std::vector<std::string>::const_iterator it = std::find(
            file_location->cgi_extensions.begin(), file_location->cgi_extensions.end(),
            response->GetRequestFileExtension()
        );
        if (it != file_location->cgi_extensions.end())
        {
            if (method != "POST" && method != "GET" && method != "HEAD")
                return SendErrorResponse(HttpStatus(STATUS_METHOD_NOT_ALLOWED), response);
            ResponseCGI* CGIresponse = new ResponseCGI(*response);
            delete response;
            return ProcessCGI(CGIresponse);
        }
    }
    if (response->GetRequest().getBodyFd() >= 0)
        close(response->GetRequest().getBodyFd());
    // HTTP simple response:
    if (method == "GET")
    {
        return ProcessGET(response);
    }
    else if (method == "HEAD")
    {
        return ProcessHEAD(response);
    }
    else if (method == "DELETE")
    {
        return ProcessDELETE(response);
    }
    else
    {
        return SendErrorResponse(HttpStatus(STATUS_METHOD_NOT_ALLOWED), response);
    }
}

void ServerClient::SendErrorResponse(const HttpStatus& status, Response* response)
{
    VirtualServerIterator virtual_server = response->GetRequest().getRequestVirtualServer();
    Response*             error_response = new Response();
    if (response->GetRequest().getBodyFd() >= 0)
        close(response->GetRequest().getBodyFd());
    // error_response->SetClientSocketFd(_client_socket_fd);
    // error_response->SetVirtualServer(response->reqGetVirtualServer());
    delete response;
    error_response->SetStatusHeaders(status.message);
    const std::map<uint16_t, std::string>&          error_pages = virtual_server->error_pages;
    std::map<uint16_t, std::string>::const_iterator it          = error_pages.find(status.code);
    struct stat                                     error_file_stat;

    if (it == error_pages.end() || stat(it->second.c_str(), &error_file_stat) != 0)
    {
        std::cerr
            << ">>>> Corresponding page template for error status: \"" << status.message
            << "\" is not provided.\n>>>> sending only headers with the error message as content."
            << std::endl;
        error_response->AppendToResponseBuff(
            std::vector<uint8_t>(status.message, status.message + strlen(status.message))
        );
    }
    else
    {
        int error_page_fd = open(it->second.c_str(), O_RDONLY);
        if (error_page_fd >= 0)
        {
            error_response->SetRequestFileFd(error_page_fd);
            error_response->SetRequestFileStat(error_file_stat);
        }
        else
            error_response->AppendToResponseBuff(
                std::vector<uint8_t>(status.message, status.message + strlen(status.message))
            );
    }
    error_response->FinishResponse();
    _responses_queue.push(error_response);
}

/*Getters & Setters*/
int ServerClient::GetClientSocketfd() const
{
    return _client_socket_fd;
}
void ServerClient::SetClientSocketfd(int client_socket_fd)
{
    _client_socket_fd = client_socket_fd;
}
int ServerClient::GetAddressSocketfd() const
{
    return _address_socket_fd;
}
void ServerClient::SetAddressSocketfd(int address_socket_fd)
{
    _address_socket_fd = address_socket_fd;
}
bool ServerClient::isStarted() const
{
    return _is_started;
}
