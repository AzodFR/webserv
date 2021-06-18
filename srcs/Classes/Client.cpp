#include "Client.hpp"

Client::Client()
{
}

Client::Client(int id, int socket)
{
	_id = id;
	_socket = socket;
}

Client::Client(const Client &ref)
{
	(void)ref;
}

Client &Client::operator=(const Client &ref)
{
	_id = ref._id;
	_content = ref._content;
	_socket = ref._socket;
	_path = ref._path;
	return *this;
}

Client::~Client()
{
}
