#include "ConfigLexer.hpp"
#include "Server.hpp"
#include "Test.hpp"
#include <csignal>

#define DEFAULT_CONFIG_PATH "config/default.conf"
#define USAGE(program_name) "Usage: " + std::string(program_name) + " [path to config file]"

extern int webserv_unix_signal;

void handle_sigint(int signal)
{
    webserv_unix_signal = signal;
    std::cout << std::endl << "Got interrupt signal." << std::endl;
}
void handle_signals()
{
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR)
        throw std::runtime_error("signal() failed.");
    if (signal(SIGQUIT, SIG_IGN) == SIG_ERR)
        throw std::runtime_error("signal() failed.");
    if (signal(SIGINT, &handle_sigint) == SIG_ERR)
        throw std::runtime_error("signal() failed.");
}
int main(const int ac, const char** av)
{
    if (ac != 2)
    {
        std::cerr << USAGE(av[0]) << std::endl;
        return 1;
    }
    try
    {
        handle_signals();
        Config::getInstance().loadConfig(ac == 2 ? av[1] : DEFAULT_CONFIG_PATH);
        Server& server = Server::GetInstance();
        server.Start();
        IOMultiplexer::GetInstance().StartEventLoop();
        std::cout << PROGNAME "/" PROGVERSION " exited." << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal Error. reason:\n" << e.what() << std::endl;
    }
    return 0;
}
