#include "Worker.hpp"

Worker::Worker()
{
	is_available = true;
	this->server = NULL;
	this->socket = 0;
	thread = new pthread_t;
}

Worker::Worker(int id)
{
	this->id = id;
	this->socket = 0;
	this->is_available = true;
	this->thread = new pthread_t;
	this->server = NULL;
}

Worker::~Worker()
{
	delete thread;
}

bool Worker::getStatus()
{
	return (is_available);
}

int Worker::getId()
{
	return (id);
}

int Worker::getSocket()
{
	return (socket);
}

pthread_t *Worker::getThread()
{
	return (thread);
}

Server *Worker::getServer()
{
	return server;
}

void Worker::setStatus(bool status)
{
	is_available = status;
}

void Worker::setSocket(int sock)
{
	socket = sock;
}

void Worker::setServer(Server *server)
{
	this->server = server;
}