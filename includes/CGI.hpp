#ifndef CGI_HPP
#define CGI_HPP
#include "Utils.hpp"
#include "Server.hpp"


class Server;

class CGI
{
private:
    string      _method;
    char **_env;
    // int         _fd_write;
    // int         _fd_read;
public:
    CGI();
    ~CGI();

    string getContentFromCGI(map<string, string> header, char *path, string location);
};




#endif