#ifndef SERVER_HPP
#define SERVER_HPP
#include "Utils.hpp"
#include "Worker.hpp"
#include "Headers.hpp"
#include "Client.hpp"
#include "CGI.hpp"

using namespace std;

typedef struct s_file
{
	string content;
	int size;
} t_file;

class Worker;
class CGI;
class Server
{
public:
	typedef struct sockaddr_in sock_addr;

private:
	Server();
	sock_addr _serv_addr;
	sock_addr _cli_addr;

	int _id;
	int _socket;
	int _port;

	bool _auto_index;

	string _name;
	t_loc *_root;
	map<string, map<string, vector<string> > > _extensions;

	map<int, string> _error_pages;
	map<string, string> _mimes;
	map<int, Worker *> _workers;
	list<Client> _clients;

	fd_set _clients_fd;
	fd_set _temp_fd;

	int _count_requests;
	int _max_sd;
	int _desc_ready;
	pthread_t *thread;
	pthread_mutex_t *_logger;

public:
	Server(int);
	Server(_t_preServ, pthread_mutex_t *, map<string, string>);
	Server &operator=(Server const &ref);
	~Server();
	int getSocket() { return _socket; };
	int getId() { return _id; };
	int getMaxSD() { return _max_sd; }
	int getPort() { return _port; }
	int getDescReady() { return _desc_ready; }
	string getName() { return _name; }

	sockaddr_in getServAddr() { return _serv_addr; };
	sockaddr_in getCliAddr() { return _cli_addr; };
	sockaddr_in *getCliAddr_ptr() { return &_cli_addr; };

	void setId(int n) { _id = n; }
	void setDescReady(int nb) { _desc_ready = nb; }
	void setTempFD(fd_set &temp) { memcpy(&_temp_fd, &temp, sizeof(temp)); }
	void setMaxSD(int val) { _max_sd = val; }
	void setErrorPages(map<int, string> pages) { _error_pages = pages; }

	fd_set &getClientsFD() { return _clients_fd; }
	fd_set *getClientsFD_ptr() { return &_clients_fd; }
	fd_set &getTempFD() { return _temp_fd; }
	fd_set *getTempFD_ptr() { return &_temp_fd; }

	pthread_t *getThread() { return thread; }
	pthread_mutex_t *getLogger() { return _logger; };
	void run(map<int, Worker *> &);
	void error(const char *);
	void log(string);
	bool check_methods(t_loc *, string, string);
	t_loc *findInLoc(string, t_loc *);
	t_find findAllLoc(string);
	void handle_request(Client &);

	string GET(map<string, string>, Client &);
	string POST(map<string, string>, Client &);
	string HEAD(map<string, string>, Client &);
	string PUT(map<string, string>, Client &);
	string DELETE(map<string, string>, Client &);
	string SEND_ERROR(int, const char *);
	string readPerChunks(Client &, string, map<string, string>);

	t_file getFile(string);

	map<string, map<string, vector<string> > > getExtensions() { return _extensions; }

	// Clients part
	list<Client> &getClients() { return _clients; }
	int exists(int socket, list<Client> &clients);

	// Workers
	int findAvailableWorker(map<int, Worker *> &);
};

#endif