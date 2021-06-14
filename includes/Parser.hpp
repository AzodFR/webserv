#ifndef PARSER_HPP
# define PARSER_HPP

#include "Utils.hpp"
#include "Server.hpp"
#include "Worker.hpp"
#include "Client.hpp"
using namespace std;



class Worker;

class Parser
{
	private:
		int		_count_workers;
		vector<_t_preServ> _pre_Serv;
		
		Parser();
		void setWorkers(string);
		
		void parseServer(int, string, string, int *);
		vector<string> parseMethod(map<string, string>, string, string, int);
		vector<string> parseMethod(string, string, int);
		void parsePort(vector<int> *, string, string, int);
		void addToLoc(map<string, string> ,vector<string>,string, t_loc *, string, int);
		
map<string, string> parseLocation(int , int *, string , string );
pair<string, map<string, vector<string> > > parseExtension(int, int *, string , string, string);
void finishLoc(t_loc *, t_loc *);
		int _i;

	public:
		Parser(string);
		Parser &operator=(const Parser &);
		~Parser();

		
		// Workers
		int getCountWorkers();
		vector<_t_preServ> getPreServ();
		

};

#endif