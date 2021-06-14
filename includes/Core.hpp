#ifndef CORE_HPP
# define CORE_HPP

#include "Utils.hpp"
#include "Server.hpp"
#include "Worker.hpp"
#include "Client.hpp"
#include "Parser.hpp"
using namespace std;



class Worker;
class Core
{
	private:
		Parser _parse;
		vector<Server *> _servers;
		map<int, Worker *> _workers;
		
		Core();
		int _i;
	public:
		Core(string);
		Core &operator=(const Core &);
		~Core();

		
		// Workers
		int getCountWorkers();
		map<int, Worker *> &getWorkers();

	
		// Server part
		void run(map<int, Worker *> & workers, int count);
		vector<Server *> getServers();
		Server *getAt(int n) { return _servers[n]; }
		void setIdServer(int i) { _i = i; }
		int getIdServer() { return _i; }

		// Client part
		
		

};

#endif