#include "Core.hpp"
// PARSIN =================================================================================================

Core::Core(string path) : _parse(path)
{
	pthread_mutex_t *logger = new pthread_mutex_t;
	pthread_mutex_init(logger, NULL);

	ifstream file("srcs/MimeTypes");
 	string str;
	size_t pos;
	string key;

	map<string, string>mimes;
  	while (getline(file, str)) {
		while ((pos = str.find(" ")) != string::npos)
		{
			key = "." + str.substr(0, pos);
			str.erase(0, pos + 1);
		}
		mimes.insert(make_pair(key, str));
  }
	for (size_t j = 0; j < _parse.getPreServ().size(); j++)
		_servers.push_back(new Server(_parse.getPreServ()[j], logger, mimes));
	for (int i = 0; i < _parse.getCountWorkers(); i++)
	{
		Worker *worker = new Worker(i);
		_workers.insert(make_pair(i, worker));
	}
}

Core::~Core()
{
	//close(_fd);
	// for (int i = 0; i < getCountWorkers(); i++)
	// 	delete this->getWorkers().find(i)->second->getThread();
}

// Hearth of webserv
// void Core::run(map<int, Worker *> &workers, int count)
// {
// 	struct timeval tv;
// 	tv.tv_usec = 0;
// 	tv.tv_sec = 20;
// 	// _count_requests = 0;
// 	// (void)workers[count];

// 	while (1)
// 	{
// 		size_t i = 0;
// 		for (i = 0; i < getServers().size(); i++)
// 		{
// 			// Prepare variables for current server
// 			socklen_t clilen = sizeof(getServers().at(i)->getCliAddr());
// 			sockaddr_in cli_addr = getServers().at(i)->getCliAddr();
// 			fd_set write_fds = getServers().at(i)->getWriteFD();
// 			fd_set read_fds = getServers().at(i)->getReadFD();
// 			fd_set write_fds2 = getServers().at(i)->getWriteFD();
// 			fd_set read_fds2 = getServers().at(i)->getReadFD();
// 			for (int l = 0; l < count; l++)
// 				workers.find(l)->second->setSocket(0);
// 			if (select(getServers().at(i)->getSocket() + 1, &read_fds2, &write_fds2, NULL, &tv) > 0)
// 			{
// 				// Search for an available worker
// 				int j;
// 				while (1)
// 				{
// 					j = 0;

// 					while (workers.find(j)->second->getStatus() && j < count - 1)
// 						j++;
// 					if (workers.find(j)->second->getStatus())
// 						break;
// 				}
// 				workers.find(j)->second->setServer(getServers().at(i));
// 				workers.find(j)->second->setSocket(22);
// 				if (!FD_ISSET(getServers().at(i)->getSocket(), &write_fds) || !FD_ISSET(getServers().at(i)->getSocket(), &read_fds))
// 					// error("ERROR non-set socket");
// 					;
// 				int newsockfd = accept(getServers().at(i)->getSocket(), (struct sockaddr *)&cli_addr, &clilen);
// 				// if (newsockfd < 0)
// 				// 	error("ERROR on accept");
// 					cout << "jen ressors jamais1 " << endl;
// 				getServers().at(i)->handle_request(newsockfd);
// 				cout << "jen ressors jamais2 " <<endl;
// 				close(newsockfd);
// 			}
// 			else
// 				getServers().at(i)->log("\e[1;96m[IDLING]\e[0m");
// 		}
// 	}
// }

Core &Core::operator=(const Core &other)
{
	//if (other == this)
	//	return *this;
	_servers = other._servers;
	_workers = other._workers;
	return *this;
}

int Core::getCountWorkers() { return _parse.getCountWorkers(); }
map<int, Worker *> &Core::getWorkers() { return _workers; }
vector<Server *> Core::getServers() { return _servers; }