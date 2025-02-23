#include "Request.hpp"
#include "Http.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <sstream>

Request::Request(const int& client_address_socket_fd)
    : _client_address_socket_fd(client_address_socket_fd)
{
    clear();
}

Request::Request(const Request& other) : _client_address_socket_fd(other._client_address_socket_fd)
{
    *this = other;
}

Request& Request::operator=(const Request& other)
{
    if (this == &other)
        return *this;
    _client_address_socket_fd        = other._client_address_socket_fd;
    _is_headers_completed            = other._is_headers_completed;
    _is_body_completed               = other._is_body_completed;
    _is_chunked                      = other._is_chunked;
    _remove_chunk_data_trailing_crlf = other._remove_chunk_data_trailing_crlf;
    _chunk_size                      = other._chunk_size;
    _remaining_chunk_size            = other._remaining_chunk_size;
    _raw_buffer                      = other._raw_buffer;
    _query_params                    = other._query_params;
    _query_params_string             = other._query_params_string;
    _method                          = other._method;
    _uri                             = other._uri;
    _http_version                    = other._http_version;
    _body_fd                         = other._body_fd;
    _body_size                       = other._body_size;
    _body_buff                       = other._body_buff;
    _body_received                   = other._body_received;
    _headers                         = other._headers;
    _status                          = other._status;
    _request_virtual_server          = other._request_virtual_server;
    _request_file_location           = other._request_file_location;
    _start_time                      = other._start_time;
    _is_receiving                    = other._is_receiving;

    _headers_raw_buf.str(std::string());
    _headers_raw_buf.clear();
    _headers_raw_buf << other._headers_raw_buf.rdbuf();
    return *this;
}

void Request::clear()
{
    _request_virtual_server          = Server::GetInstance().GetConfig().begin();
    _status                          = HttpStatus(STATUS_OK);
    _is_headers_completed            = false;
    _is_body_completed               = false;
    _is_chunked                      = false;
    _remove_chunk_data_trailing_crlf = false;
    _chunk_size                      = 0;
    _remaining_chunk_size            = 0;
    _body_fd                         = -1;
    _body_size                       = 0;
    _body_received                   = 0;
    _is_receiving                    = false;

    _raw_buffer.clear();
    if (_body_buff.size())
    {
        _raw_buffer.insert(_raw_buffer.end(), _body_buff.begin(), _body_buff.end());
        _body_buff.clear();
    }
    _headers.clear();
    _http_version.clear();
    _method.clear();
    _uri.clear();
    _query_params_string.clear();
    _query_params.clear();
    _headers_raw_buf.str(std::string());
    _headers_raw_buf.clear();
}

Request::~Request() {}

Request& Request::operator+=(const std::vector<uint8_t>& bytes)
{
    _is_receiving = true;
    _start_time   = time(0);
    if (_is_headers_completed == false)
    {
        _raw_buffer.insert(_raw_buffer.end(), bytes.begin(), bytes.end());
        const char* headers_end = CRLF CRLF;
        const char*                    found =
            utils::strnstr((const char*)_raw_buffer.data(), headers_end, _raw_buffer.size());

        if (found != NULL)
        {
            size_t headers_end_pos =
                (found - (const char*)_raw_buffer.data()) + strlen(headers_end);
            if (headers_end_pos < _raw_buffer.size())
            {
                _body_buff.insert(
                    _body_buff.end(), _raw_buffer.begin() + headers_end_pos, _raw_buffer.end()
                );
                _raw_buffer.erase(_raw_buffer.begin() + headers_end_pos, _raw_buffer.end());
            }
            _raw_buffer.insert(_raw_buffer.end(), 0);
            _headers_raw_buf.clear();
            _headers_raw_buf.str((char*)_raw_buffer.data());
            _raw_buffer.clear();
            parse();
        }
        else if (_raw_buffer.size() > REQUEST_HEADER_LIMIT)
        {
            _status               = HttpStatus(STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE);
            _is_headers_completed = true;
            _is_body_completed    = true;
        }
    }
    else if (_is_body_completed == false)
    {
        _body_buff.insert(_body_buff.end(), bytes.begin(), bytes.end());
        if (_is_chunked)
        {
            writeChunked();
        }
        else
        {
            if (_body_buff.size())
                writeBodyToFile();
        }
    }
    else
    {
        assert(!"IMPOSSIBLE TO REACH!");
    }

    return *this;
}
void Request::writeBodyToFile()
{
    size_t  to_write = std::min(_body_buff.size(), (_body_size - _body_received));
    ssize_t bytes_written;
    if (_body_fd >= 0)
        bytes_written = write(_body_fd, _body_buff.data(), to_write);
    else
        bytes_written = to_write;
    if (bytes_written < 0 || (size_t)bytes_written != to_write)
    {
        std::cerr << "write(): failed to write received request body." << std::endl;
        _status = HttpStatus(STATUS_INTERNAL_SERVER_ERROR);
        close(_body_fd);
        _body_fd = -1;
    }
    _body_received += to_write;
    _body_buff.erase(_body_buff.begin(), _body_buff.begin() + to_write);
    if (_body_size > _request_file_location->max_body_size)
    {
        close(_body_fd);
        _body_fd = -1;
        _status  = HttpStatus(STATUS_REQUEST_ENTITY_TOO_LARGE);
    }
    if (_body_received == _body_size)
    {
        std::cerr << ">>>> Received: " << _body_size << " Bytes of request body." << std::endl;
        _is_body_completed = true;
        if (_body_fd >= 0 && lseek(_body_fd, 0, SEEK_SET) == -1)
        {
            std::cerr << "lseek(): failed to write received request body." << std::endl;
            close(_body_fd);
            _body_fd = -1;
            _status  = HttpStatus(STATUS_INTERNAL_SERVER_ERROR);
        }
    }
}
void Request::writeChunkToFile()
{
    size_t to_write = std::min(_remaining_chunk_size, _body_buff.size());
    if (to_write)
    {
        ssize_t bytes_written;
        if (_body_fd >= 0)
            bytes_written = write(_body_fd, _body_buff.data(), to_write);
        else
            bytes_written = to_write;
        if (bytes_written < 0 || to_write != (size_t)bytes_written)
        {
            std::cerr << "write(): failed to write received request body." << std::endl;
            close(_body_fd);
            _body_fd = -1;
            _status  = HttpStatus(STATUS_INTERNAL_SERVER_ERROR);
        }
        _body_size += to_write;
        _remaining_chunk_size -= to_write;
        _body_buff.erase(_body_buff.begin(), _body_buff.begin() + to_write);
    }
    if (_body_size > _request_file_location->max_body_size)
    {
        close(_body_fd);
        _body_fd = -1;
        _status  = HttpStatus(STATUS_REQUEST_ENTITY_TOO_LARGE);
    }
}
void Request::writeChunked()
{
    if (_remaining_chunk_size)
    {
        writeChunkToFile();
        if (!_remaining_chunk_size)
        {
            if (_body_buff.size() >= 2)
            {
                _body_buff.erase(_body_buff.begin(), _body_buff.begin() + 2);
                _remove_chunk_data_trailing_crlf = false;
            }
            else
            {
                _remove_chunk_data_trailing_crlf = true;
                return;
            }
        }
    }
    if (!_remaining_chunk_size && _remove_chunk_data_trailing_crlf)
    {
        if (_body_buff.size() < 2)
            return;
        _body_buff.erase(_body_buff.begin(), _body_buff.begin() + 2);
        _remove_chunk_data_trailing_crlf = false;
    }
    if (_body_buff.size())
    {
        const char* chunk_size_ln =
            utils::strnstr((const char*)_body_buff.data(), CRLF, _body_buff.size());
        if (chunk_size_ln)
        {
            _chunk_size           = std::strtoul((const char*)_body_buff.data(), NULL, 16);
            _remaining_chunk_size = _chunk_size;
            _body_buff.erase(
                _body_buff.begin(),
                _body_buff.begin() + (chunk_size_ln + 2 - (const char*)_body_buff.data())
            );
            for (; _body_buff.size() && chunk_size_ln && _chunk_size;)
            {
                writeChunkToFile();
                if (!_remaining_chunk_size)
                {
                    if (_body_buff.size() > 2)
                        _body_buff.erase(_body_buff.begin(), _body_buff.begin() + 2);
                    else
                    {
                        _remove_chunk_data_trailing_crlf = true;
                        break;
                    }
                }
                chunk_size_ln =
                    utils::strnstr((const char*)_body_buff.data(), CRLF, _body_buff.size());
                if (chunk_size_ln)
                {
                    _chunk_size           = std::strtoul((const char*)_body_buff.data(), NULL, 16);
                    _remaining_chunk_size = _chunk_size;
                    _body_buff.erase(
                        _body_buff.begin(),
                        _body_buff.begin() + (chunk_size_ln + 2 - (const char*)_body_buff.data())
                    );
                }
            }
        }
    }
    if (_chunk_size == 0 && _body_buff.size())
    {
        const char* trailing_line =
            utils::strnstr((const char*)_body_buff.data(), CRLF, _body_buff.size());
        if (trailing_line)
        {
            std::cerr << ">>>> Received: " << _body_size << " Bytes of chunked request body."
                      << std::endl;
            _body_buff.erase(
                _body_buff.begin(),
                _body_buff.begin() + (trailing_line + 2 - (const char*)_body_buff.data())
            );
            _is_body_completed = true;
            if (_body_fd >= 0 && lseek(_body_fd, 0, SEEK_SET) == -1)
            {
                std::cerr << "lseek(): failed to write received request body." << std::endl;
                close(_body_fd);
                _body_fd = -1;
                _status  = HttpStatus(STATUS_INTERNAL_SERVER_ERROR);
            }
        }
    }
}

void Request::parse()
{
    _is_headers_completed = true;
    try
    {
        parseRequestLine();
        parseHeaders();
        // Getting Requested File Location depending on the Server Config:
        {
            _request_virtual_server =
                ServerUtils::GetRequestVirtualServer(_client_address_socket_fd, *this);
            _request_file_location = ServerUtils::GetFileLocation(_request_virtual_server, _uri);
        }
        // Buffering Request Body Content To A temp file:
        {
            const HttpHeader* transfer_encoding_header = getHeader("Transfer-Encoding");
            if (transfer_encoding_header != NULL &&
                transfer_encoding_header->values.front().value ==
                    "chunked") // TODO: this should be case insensitive and check through all header
                               // values
            {
                _is_chunked = true;
                _body_fd    = open("/tmp/", O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
                if (_body_fd < 0)
                    _status = HttpStatus(STATUS_INTERNAL_SERVER_ERROR);
                if (_body_buff.size())
                    writeChunked();
            }
            else
            {
                const HttpHeader* lenght_header = getHeader("Content-Length");
                if (lenght_header == NULL)
                {
                    _body_size         = 0;
                    _is_body_completed = true;
                }
                else
                {
                    _body_size = strtoul(lenght_header->raw_value.c_str(), NULL, 10);
                    _body_fd   = open("/tmp/", O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
                    if (_body_fd < 0)
                        _status = HttpStatus(STATUS_INTERNAL_SERVER_ERROR);
                    if (_body_buff.size())
                        writeBodyToFile();
                }
            }
        }
    }
    catch (const RequestException& e)
    {
        _status            = e.getErrorCode();
        _is_body_completed = true;
    }
}

const std::string Request::getMethod() const
{
    return _method;
}

const HttpStatus& Request::getStatus() const
{
    return _status;
}

bool Request::isCompleted() const
{
    return _is_headers_completed && _is_body_completed;
}
bool Request::isChunked() const
{
    return _is_chunked;
}

VirtualServerIterator Request::getRequestVirtualServer() const
{
    return _request_virtual_server;
}

LocationIterator Request::getRequestFileLocation() const
{
    return _request_file_location;
}

bool Request::getIsReceiving() const
{
    return _is_receiving;
}

time_t Request::getStartTime() const
{
    return _start_time;
}

std::string Request::getUri() const
{
    return _uri;
}

std::string Request::getVersion() const
{
    return _http_version;
}

const HttpHeader* Request::getHeader(const std::string& key) const
{
    std::vector<HttpHeader>::const_iterator it = _headers.begin();
    for (; it != _headers.end(); it++)
    {
        if (it->name == key)
            return it.base();
    }
    return NULL;
}

const std::vector<HttpHeader>& Request::getHeaders() const
{
    return _headers;
}

int Request::getBodyFd() const
{
    return _body_fd;
}
size_t Request::getBodySize() const
{
    return _body_size;
}
const std::map<std::string, std::string>& Request::getQueryParams() const
{
    return _query_params;
}
const std::string& Request::getQueryParamsString() const
{
    return _query_params_string;
}

const std::stringstream& Request::getRawBuffer() const
{
    return _headers_raw_buf;
}

void Request::setMethod(const std::string& method)
{
    _method = method;
}

void Request::setUri(const std::string& uri)
{
    if (uri.empty() || uri[0] != '/')
        throw RequestException(HttpStatus(STATUS_BAD_REQUEST));
    _uri = uri;
}

void Request::setVersion(const std::string& version)
{
    if (version.substr(0, 5) != "HTTP/")
        throw RequestException(HttpStatus(STATUS_BAD_REQUEST));

    float http_version;
    if (std::sscanf(version.c_str() + 5, "%f", &http_version) != 1)
        throw RequestException(HttpStatus(STATUS_BAD_REQUEST));
    if (http_version != 1.1f)
        throw RequestException(HttpStatus(STATUS_HTTP_VERSION_NOT_SUPPORTED));
    _http_version = version;
}

void Request::setHeader(const HttpHeader& header)
{
    _headers.push_back(header);
}

std::string Request::getHeaderLine()
{
    std::string line;

    std::getline(_headers_raw_buf, line, '\n');
    if (line.empty() || line[line.size() - 1] != '\r')
        throw RequestException(HttpStatus(STATUS_BAD_REQUEST));
    line.erase(line.size() - 1);

    if (std::string(" \t").find(_headers_raw_buf.peek()) != std::string::npos)
    {
        _headers_raw_buf.ignore();
        line += ' ';
        while (std::string(" \t").find(_headers_raw_buf.peek()) != std::string::npos)
            _headers_raw_buf.ignore();
        line += Request::getHeaderLine();
    }
    return line;
}

void Request::parseRequestLine()
{
    std::string method, uri, version;

    if (!std::getline(_headers_raw_buf, method, ' ') || !std::getline(_headers_raw_buf, uri, ' '))
    {
        throw RequestException(HttpStatus(STATUS_BAD_REQUEST));
    }

    if (uri.find('?') != std::string::npos)
    {
        _query_params_string = uri.substr(uri.find('?') + 1);
        uri                  = uri.substr(0, uri.find('?'));
        _query_params        = parseQueryParams(_query_params_string);
    }

    if (!std::getline(_headers_raw_buf, version, '\n') || version[version.size() - 1] != '\r')
    {
        throw RequestException(HttpStatus(STATUS_BAD_REQUEST));
    }

    version.erase(version.size() - 1);

    setMethod(method);
    setUri(uri);
    setVersion(version);
}

std::map<std::string, std::string> Request::parseQueryParams(const std::string query)
{
    std::map<std::string, std::string> params;

    std::string::const_iterator start = query.begin();
    std::string::const_iterator it    = query.begin();
    std::string                 key, value;
    for (; it != query.end(); ++it)
    {
        if (*it == '=' && start != it)
        {
            key   = utils::strtrim(std::string(start, it));
            start = it + 1;
        }
        else if ((*it == '&' || it + 1 == query.end()) && !key.empty())
        {
            value       = utils::strtrim(std::string(start, it));
            params[key] = value;
            start       = it + 1;
            key.clear();
            value.clear();
        }
    }
    return params;
}

void Request::parseHeaders()
{
    std::string line;
    while (!(line = Request::getHeaderLine()).empty())
    {
        size_t pos = line.find(':');
        if (pos == std::string::npos)
            throw RequestException(HttpStatus(STATUS_BAD_REQUEST));
        std::string name  = utils::strtrim(line.substr(0, pos)),
                    value = utils::strtrim(line.substr(pos + 1));
        HttpHeader header(name, value);
        setHeader(header);
    }
}

std::ostream& operator<<(std::ostream& os, const Request& request)
{
    os << "===========================================================\n";
    os << "Method: " << request.getMethod() << '\n';
    os << "URI: " << request.getUri() << '\n';
    os << "Query string: " << request.getQueryParamsString() << '\n';
    os << "Query params: " << '\n';
    for (size_t i = 0; i < request.getQueryParams().size(); i++)
    {
        os << "- " << request.getQueryParams().begin()->first << " = "
           << request.getQueryParams().begin()->second << '\n';
    }
    os << "HTTP version: " << request.getVersion() << '\n';
    os << "Headers:\n";

    size_t max_length = 0;

    std::vector<HttpHeader>::const_iterator it;
    for (it = request.getHeaders().begin(); it != request.getHeaders().end(); ++it)
    {
        max_length = std::max(max_length, it->name.length());
    }

    for (it = request.getHeaders().begin(); it != request.getHeaders().end(); ++it)
    {
        os << *it << '\n';
    }
    os << "===========================================================" << std::endl;
    return os;
}
