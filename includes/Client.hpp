#ifndef CLIENT_HPP
#define CLIENT_HPP
#include "Utils.hpp"

class Client
{
private:
	int _socket;
	int _id;
	string _content;
	string _path;

public:
	Client();
	Client(int, int);
	Client(const Client &);
	Client &operator=(const Client &);
	~Client();

	// Setters and getters
	int getSocket() { return _socket; }
	int getId() { return _id; }
	string getContent() { return _content; }
	string &getPath() { return _path; }

	void setSocket(int socket) { _socket = socket; }
	void setId(int id) { _id = id; }
	void setContent(string content) { _content = content; }
	void setPath(string path) { _path = path; }

	bool exists(int socket);
};

#endif