#include "CGI.hpp"

CGI::CGI()
{
}

CGI::~CGI()
{
}

string CGI::getContentFromCGI(map<string, string> header, char *path, string location)
{
    string content;
    vector<string> env;
    // Create environment for the CGI
	char path_cwd[2048];
	getcwd(path_cwd, 2048);
    env.push_back("PATH_INFO=" + string(path_cwd));
    env.push_back("SCRIPT_FILENAME=" + location);
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env.push_back("REQUEST_METHOD=" + header.find("Request-Type")->second);
    env.push_back("REDIRECT_STATUS=200");
    env.push_back("REQUEST_URI=" + string(path_cwd));

    string query;

    if (header.count("Variables"))
        query = header.find("Variables")->second;
    else if (header.count("Content"))
    {
        query = header.find("Content")->second;
    }
    //cout << "1:---\n" << query << endl << "---\n2:---\n" << header.find("Content")->second << endl;
    if (!query.size())
        query.push_back('\n');
    if (header.find("Request-Type")->second == "GET")
        env.push_back("QUERY_STRING=" + query);

    env.push_back("CONTENT_LENGTH=" + to_string(query.size()));

    if (header.count("Content-Type"))
        env.push_back("CONTENT_TYPE=" + header.find("Content-Type")->second);
    else
        env.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");

    // Prepare pipes and fd communication between parent and fork child
    int pipe1[2], ret = 0;
    pid_t child, parent = 0;

    int fd = open("tmp", O_CREAT | O_TRUNC | O_WRONLY | O_NONBLOCK, 0777);

    write(fd, query.c_str(), query.size());
    close(fd);
    size_t i = 0;

    char **yes = new char *[env.size() + 1];

    for (; i != env.size(); i++)
        yes[i] = (char *)env.at(i).c_str();
    yes[i] = NULL;
    _env = yes;

    char *echo[3] = {(char *)"cat", (char *)"tmp", NULL};

    char *cmd[] = {path, (char *)location.c_str(), NULL};
    if (pipe(pipe1) == -1)
    {
        perror("CGI part : Pipe failed");
        exit(1);
    }
    child = fork();
    int tmp = open(".tmp", O_CREAT | O_TRUNC | O_NONBLOCK | O_RDWR, 0777);

    if (child == -1)
    {
        perror("CGI part : Fork failed");
        exit(1);
    }
    else if (!child)
    {
        dup2(pipe1[1], 1);
        close(pipe1[0]);
        ret = execvp(echo[0], echo);
    }
    else
    {
        int status2;

        wait(&status2);
        parent = fork();
        if (!parent)
        {
            dup2(pipe1[0], 0);
            dup2(tmp, 1);
            close(pipe1[1]);
            ret = execve(cmd[0], cmd, _env);
        }
        else
        {
            int status;

            wait(&status);
            close(pipe1[0]);
            close(pipe1[1]);
            close(tmp);

            usleep(100000);

            tmp = open(".tmp", O_NONBLOCK | O_RDONLY);

            char buf[65535];
            bzero(buf, 65535);
            read(tmp, buf, 65535);
            close(tmp);
            content = string(buf);
            remove("tmp");
            remove(".tmp");

            delete[] yes;
            return (content);
        }
    }

    delete[] yes;
    return content;
}
