#include "Utils.hpp"
#include "Server.hpp"
#include "Headers.hpp"
#include "Core.hpp"
#include "Worker.hpp"

using namespace std;

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

void *main_loop(void *arg)
{
	Worker *w = reinterpret_cast<Worker *>(arg);

	// Infinite loop, listening on server's run() function
	while (1)
	{
		if (w->getSocket() != 0 && w->getIt() == 0 && w->getStatus() == false && w->getServer() != NULL)
		{
			w->setIt(1);
			w->setStatus(false);

			bool connection_closed = false, server_end = false;
			Server *server = w->getServer();

			// Check for an available file descriptor
			for (int fd = 1; fd <= server->getMaxSD() && server->getDescReady() > 0; fd++)
			{
				if (FD_ISSET(fd, &server->getTempFD()))
				{
					server->setDescReady(server->getDescReady() - 1);
					if (fd == server->getSocket())
					{
						int new_socket = 0;
						do
						{
							new_socket = accept(server->getSocket(), NULL, NULL);
							if (new_socket < 0)
							{
								if (errno != EWOULDBLOCK)
								{
									perror("Accept() failed : ");
									server_end = true;
								}
								break;
							}
							// Print log accept() new connection
							server->log("\e[1;94m[New incoming connection socket(" + to_string(fd) + ")]\e[0;0m");
							FD_SET(new_socket, &server->getClientsFD());
							if (new_socket > server->getMaxSD())
								server->setMaxSD(new_socket);
						} while (new_socket != -1);
					}
					else
					{
						server->log("\e[1;96m[Worker " + to_string(w->getId()) + " is working on this server!]\e[0;0m");
						fcntl(fd, F_SETFL, O_NONBLOCK);
						Client client(server->getClients().size() + 1, fd);
						int nbytes_read = 0;
						string buff;
						char buffer[100000];
						int receive = 0;

						do
						{
							bzero(buffer, 10000);
							usleep(50);
							int readed = recv(fd, buffer, 100000, 0);
							nbytes_read += readed;
							if (!readed)
							{
								if (receive >= 10000)
								{
									connection_closed = true;
									break;
								}
								else
								{
									receive++;
									continue;
								}
							}
							for (int i = 0; i < readed; i++)
								buff.push_back(buffer[i]);
							if (nbytes_read < 0)
							{
								connection_closed = true;
								client.setContent(buff);
								server->handle_request(client);
								break;
							}
							if (nbytes_read < 0)
							{
								if (errno != EWOULDBLOCK)
								{
									perror("recv() failed");
									server_end = true;
								}
								break;
							}
						} while (true);

						if (connection_closed)
						{
							server->log("\e[1;31m[Connection closed]\e[0m");
							close(fd);
							FD_CLR(fd, &server->getClientsFD());
							if (fd == server->getMaxSD())
							{
								while (!FD_ISSET(server->getMaxSD(), &server->getClientsFD()))
									server->setMaxSD(server->getMaxSD() - 1);
							}
						}
					}
				}
			}
			w->setStatus(true);
			w->setServer(NULL);
		}
	}
	return NULL;
}

void *run(void *arg)
{
	Core *core = reinterpret_cast<Core *>(arg);
	core->getServers().at(core->getIdServer())->run(core->getWorkers());
	return (NULL);
}

int main(int argc, char *argv[])
{
	string path;
	if (argc < 2)
		path = "./default/default.conf";
	else
		path = argv[1];
	Core core(path);

	// Threads for multi-workers
	for (int i = 0; i < core.getCountWorkers(); i++)
	{
		pthread_t *thread = core.getWorkers().find(i)->second->getThread();
		pthread_create(thread, NULL, main_loop, reinterpret_cast<void *>(core.getWorkers().find(i)->second));
		usleep(10);
		pthread_detach(*thread);
		pthread_join(*thread, NULL);
	}

	// Threads for multi-servers
	for (size_t i = 0; i < core.getServers().size(); i++)
	{
		core.setIdServer(i);
		pthread_t *thread = core.getServers().at(i)->getThread();
		pthread_create(thread, NULL, run, reinterpret_cast<void *>(&core));
		usleep(10);
		pthread_detach(*thread);
	}

	string line;
	while (getline(cin, line))
		if (line == "STOP")
			exit(0);

	return 0;
}