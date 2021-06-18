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

	map<string, string> mimes;
	while (getline(file, str))
	{
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
}

Core &Core::operator=(const Core &other)
{
	_servers = other._servers;
	_workers = other._workers;
	return *this;
}

int Core::getCountWorkers() { return _parse.getCountWorkers(); }
map<int, Worker *> &Core::getWorkers() { return _workers; }
vector<Server *> Core::getServers() { return _servers; }