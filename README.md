<!-- Improved compatibility of back to top link: See: https://github.com/othneildrew/Best-README-Template/pull/73 -->
<a id="readme-top"></a>
<!--
*** Thanks for checking out the Best-README-Template. If you have a suggestion
*** that would make this better, please fork the repo and create a pull request
*** or simply open an issue with the tag "enhancement".
*** Don't forget to give the project a star!
*** Thanks again! Now go create something AMAZING! :D
-->



<!-- PROJECT SHIELDS -->
<!--
*** I'm using markdown "reference style" links for readability.
*** Reference links are enclosed in brackets [ ] instead of parentheses ( ).
*** See the bottom of this document for the declaration of the reference variables
*** for contributors-url, forks-url, etc. This is an optional, concise syntax you may use.
*** https://www.markdownguide.org/basic-syntax/#reference-style-links
-->
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![project_license][license-shield]][license-url]
[![LinkedIn][linkedin-shield]][linkedin-url]



<!-- PROJECT LOGO -->
<br />
<div align="center">
  <a href="https://github.com/msitni1337/webserv">
    <img src="images/webserv.png" alt="Logo" width="80" height="80">
  </a>

<h3 align="center">Webserv</h3>

  <p align="center">
    This is an implementation of the HTTP 1.1 & CGI 1.1 protocols. developed in c++98.
    <br />
    &middot;
    <a href="https://github.com/msitni1337/webserv/issues/new?labels=bug&template=bug-report---.md">Report Bug</a>
    &middot;
    <a href="https://github.com/msitni1337/webserv/issues/new?labels=enhancement&template=feature-request---.md">Request Feature</a>
  </p>
</div>



<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#config-file">Config file</a></li>
      <ul>
        <li><a href="#config-file-overview">Config file overview</a></li>
        <li><a href="#directives-Description">Directives Description</a></li>
      </ul>
    <li><a href="#higher-architecture">Higher architecture</a></li>
    <li><a href="#classes-description">Classes description</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project

[![Product Name Screen Shot][product-screenshot]]

This project is part of the <b>42 network's common core cursus</b>.<br>
The aim of this project is to create a HTTP 1.1 compliant web server based on the <a href="https://datatracker.ietf.org/doc/html/rfc2616">HTTP/1.1 RFC (rfc2616)</a> and <a href="https://datatracker.ietf.org/doc/html/rfc3875">The Common Gateway Interface (CGI) Version 1.1 (rfc3875)</a>.<br>
The project is built purely in the c++ language respecting the 98 standard.<br>
`Webserv` was inspired from the famous web server <a href="https://github.com/nginx/nginx">Nginx</a> in the handling of some cases of the protocol.
<p align="right">(<a href="#readme-top">back to top</a>)</p>


<!-- GETTING STARTED -->
## Getting Started

This is how you can build & setup `Webserv` on your machine to serve your website files from your local machine to the outside world of internet. Don't forget to give the project a star!

### Prerequisites

* Install required build tools: `make and c++`
  ```sh
  sudo apt install make g++ c++
  ```

### Installation

1. Clone the repo:
   ```sh
   git clone https://github.com/msitni1337/webserv.git
   ```
2. Build the project:
   ```sh
   cd webserv
   make
   ```
3. Create your configuration file. To learn more check out the section <a href="#config-file">Config file</a>.
4. Run Webserv with your config or use the test one located at `./config/server.conf`.
   ```sh
   ./webserv path/to/config/file
   ```
5. Navigate with your browser to your local address with the port specified in the configuration file to browse your website.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- USAGE EXAMPLES -->
## Config file

### Config file overview

The config file is inspired from the nginx conf file however as every project we do things a little bit differently.<br>
Check out `./config/server.conf` for a complete config file example with all features included.

The config file is hierarchical, `Webserv` parses the config file in a blocks style.<br>
`Webserv` recognizes the following 2 blocks directives: `server` & `location` those blocks must be started with an open brace and ended with a closed brace, failure to do so will result in a parsing failure.<br>

* `server` directive tells `Webserv` to create a new virtual server, it can contain one or multiple `location` blocks.
  ```
  server{
    listen 80;
    
    location {
      ...
    }
  }
  ```
* `location` directive tells `Webserv` to parse a new location inside the virtual server which can have different directive values from the virtual server it resides on.
  ```
  server{
    listen 80;
    root /default/root/path
    
    location {
      root /different/root/path
      ...
    }
  }
  ```

### Directives Description

Here is a description of every directive in alphabetical order that can be included inside the parsing blocks.

* `allow_methods`:
  Specify an array of allowed methods any method not included will not be handled.
  ```
   allow_methods GET POST;
   allow_methods [[GET|POST|DELETE] ..];
  ```

* `autoindex`:
  Toggle autoindex on or off for listing files in directories with no matched index file.
  Default: off.
  ```
   autoindex on;
   autoindex on|off;
  ```

* `cgi_extensions`:
  Specify an array of request's scripts extensions that will be passed to the cgi executable if rquested
  else file extensions not included will be served normally.
  ```
  cgi_extensions php;
  cgi_extensions [[file_extension] ..];
  ```

* `cgi_path`:
  Specify the path to the cgi executable that will execute the cgi scripts.
  ```
  cgi_path /usr/bin/php-cgi;
  cgi_path path/to/cgi/executable;
  ```

* `client_max_body_size`:
  Sets the maximum body data clients can sent to the server in each request.<br>
  You can specify B: bytes, K: kilobytes, M: megabytes, G: gigabytes.
  ```
   client_max_body_size 200 M;
   client_max_body_size Number [B|K|M|G];
  ```

* `error_page`:
  Sets a custom error page for Webserv to respond.
  ```
   error_page 404 ./pages/404.html;
   error_page ERROR_CODE FILE_TO_SERV;
  ```

* `include`:
  Include another config file (best for distributing large configs works like the c preproccessor):
  ```
   include ./mime.types;
   include path/to/file;
  ```

* `index`:
  Provide index filenames to match when client request a directory if no match occure either listing directory path or forbidden is sent to client depending on directive autoindex.
  ```
   error_page 404 ./pages/404.html;
   error_page ERROR_CODE FILE_TO_SERV;
  ```

* `listen`:
  Makes the virtual server listen for incoming connections and request on address:port.
  ```
   listen 0.0.0.0:8080;
   listen address:port;
  ```

* `return`:
  Returns a simple response with `http_header_code` and a `url`:
  ```
   return 301 /blog;
   return CODE URL;
  ```

* `root`:
  Sets the root from where Webserv will search for files to serve from.
  ```
   root ./locations/site;
   root path/to/root;
  ```

* `server_name`:
  This directive is matched with the `host` header in the requests to determine targeted virtual server.
  ```
   server_name example.com www.example.com;
   server_name website_url [[website_url] .. ];
  ```

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ROADMAP -->
## Higher architecture

[![Event driven architecture][classes-flowchart]]

`Webserv` uses <b>Event-driven architecture</b> to acheive flexibility of code flow while making `Webserv` able to handle the maximum traffic possible. To do so, `Webserv` needs to solve the blocking problem of calls to `read/write/recv/send` when used on a blocking file descriptors (fd) such as `pipes` or `sockets`. These special files descriptors in constrast to regular one (fds of local files for example) blocks the execution of the code until the fd is ready. inherently they are not ready to read or write by default, calling `recv()` for example on a file descriptor linked to the end-user (client) socket will make the server halt until DATA is ready to be received from the client. `Webserv` solves this problem by routing all traffic through the `epoll` family functions learn more about them <a href="https://man7.org/linux/man-pages/man7/epoll.7.html">here</a> or `man epoll`.<br>
Notably like shown in the flowchart, `Webserv` uses the class `IOMultiplixer` as a gateway for everyone to subscribe to it's event loop by providing a blocking file descriptor to listen for. By inheriting from the interface `AIOEventListener` classes can use this interface to hook into `IOMultiplixer` API. This abstracts away the low-level `epoll` API functions and makes `Webserv`'s code more flexible to work with. Now any class who want to route it's `read/write/recv/send` calls through epoll for efficent operations can do so seamlessly through `IOMultiplixer` new API. Indeed `Webserv` uses this concept to make The `Server` class listen on sockets and handle new connection, in the other hand each new connection uses a pool of `ServerClient`s to spawn an instance of `ServerClient` that can efficently handle the client requests, all while making sure all the blocking operations are routed through `IOMultiplixer` to make sure `Webserv` is ready to do the selected operation and garantee maximum effeciency and speed.<br>
This approach also takes in mind upcoming features development which can be easily made to the system as this method allows many Classes to plug into the eco-system of the `IOMultiplixer`.
<p align="right">(<a href="#readme-top">back to top</a>)</p>



## Classes description

<!-- CONTRIBUTING -->
## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<p align="right">(<a href="#readme-top">back to top</a>)</p>

### Top contributors:

<a href="https://github.com/msitni1337/webserv/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=msitni1337/webserv" alt="contrib.rocks image" />
</a>



<!-- LICENSE -->
## License

Distributed under the MIT License. See `LICENSE.txt` for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- CONTACT -->
## Contact

Mohammed Sitni - msitni1337@gmail.com

Project Link: [https://github.com/msitni1337/webserv](https://github.com/msitni1337/webserv)

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ACKNOWLEDGMENTS -->
## Acknowledgments

* [Oussama benchekroun](https://github.com/obenchkroune) Team mate with whome I worked in the project.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/msitni1337/webserv.svg?style=for-the-badge
[contributors-url]: https://github.com/msitni1337/webserv/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/msitni1337/webserv.svg?style=for-the-badge
[forks-url]: https://github.com/msitni1337/webserv/network/members
[stars-shield]: https://img.shields.io/github/stars/msitni1337/webserv.svg?style=for-the-badge
[stars-url]: https://github.com/msitni1337/webserv/stargazers
[issues-shield]: https://img.shields.io/github/issues/msitni1337/webserv.svg?style=for-the-badge
[issues-url]: https://github.com/msitni1337/webserv/issues
[license-shield]: https://img.shields.io/github/license/msitni1337/webserv.svg?style=for-the-badge
[license-url]: https://github.com/msitni1337/webserv/blob/master/LICENSE.txt
[linkedin-shield]: https://img.shields.io/badge/-LinkedIn-black.svg?style=for-the-badge&logo=linkedin&colorB=555
[linkedin-url]: https://linkedin.com/in/msitni
[product-screenshot]: images/screenshot.png
[classes-flowchart]: images/classes_flowchart.png