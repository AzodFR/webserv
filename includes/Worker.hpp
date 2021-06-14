#ifndef WORKERS_HPP
#define WORKERS_HPP
#include "Headers.hpp"
#include "Server.hpp"

class Server;
class Worker
{
	typedef struct s_file
{
	string content;
	size_t size;
}				t_file;
private:
	Server *server;
	pthread_t *thread;
	bool is_available;
	int socket;
	int id;
	int it;

public:
	Worker();
	Worker(int id);
	~Worker();

	// Getters and setters
	bool getStatus();
	int getId();
	int getSocket();
	int getIt() { return it; }
	Server *getServer();
	pthread_t *getThread();
	void setStatus(bool);
	void setSocket(int);
	void setServer(Server *);
	void setIt(int oui) { it = oui;}

};



#endif