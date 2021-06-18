#include "Server.hpp"
#include "Utils.hpp"
#include "Client.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

Server::Server(int port)
{
	//Socket()
	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket < 0)
		perror("ERROR opening socket");

	//IOCTL()
	const int opt = 1;
	setsockopt(_socket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

	//BIND()
	bzero((char *)&_serv_addr, sizeof(_serv_addr));
	_serv_addr.sin_family = AF_INET;
	_serv_addr.sin_addr.s_addr = INADDR_ANY;
	_serv_addr.sin_port = htons(port);
	if (bind(_socket, (struct sockaddr *)&_serv_addr, sizeof(_serv_addr)) < 0)
		perror("ERROR on binding");
	listen(_socket, 4096);
}

Server::Server(_t_preServ pre, pthread_mutex_t *logger, map<string, string> mimes)
{
	// Init thread
	_id = pre.id;
	_name = pre.name;
	_logger = logger;

	_port = pre.port;

	log("\e[33;1m[Starting ...]");
	thread = new pthread_t;
	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket < 0)
		perror("ERROR opening socket");

	//IOCTL()
	int opt = 1;
	setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
	// ioctl(_socket, FIONBIO, (char *)&opt);
	fcntl(_socket, F_SETFL, O_NONBLOCK);
	//BIND()
	bzero((char *)&_serv_addr, sizeof(_serv_addr));
	_serv_addr.sin_family = AF_INET;
	_serv_addr.sin_addr.s_addr = INADDR_ANY;
	_serv_addr.sin_port = htons(pre.port);
	if (bind(_socket, (struct sockaddr *)&_serv_addr,
			 sizeof(_serv_addr)))
		error("ERROR on binding");

	listen(_socket, 4096);

	FD_ZERO(&_clients_fd);
	_max_sd = _socket;
	FD_SET(_socket, &_clients_fd);

	_error_pages = pre.err;
	_root = pre._root;
	_extensions = pre.ext;
	_auto_index = pre.auto_index;
	_mimes = mimes;
}

Server::~Server()
{
	close(_socket);
}

Server &Server::operator=(Server const &ref)
{

	_serv_addr = ref._serv_addr;
	_cli_addr = ref._cli_addr;

	_id = ref._id;
	_socket = ref._socket;

	_name = ref._name;
	_root = ref._root;

	_error_pages = ref._error_pages;

	_root = ref._root;
	_auto_index = ref._auto_index;
	_port = ref._port;
	_extensions = ref._extensions;
	_mimes = ref._mimes;
	return *this;
}

// Hearth of the request for the server
void Server::run(map<int, Worker *> &workers)
{
	struct timeval tv = {100, 0};

	fd_set temp_fd;
	log("\e[32;1m[Started !]");
	while (1)
	{
		int retval = 0, index = 0;
		memcpy(&temp_fd, &_clients_fd, sizeof(_clients_fd));
		_desc_ready = 0;
		if ((retval = select(_max_sd + 1, &temp_fd, NULL, NULL, &tv)) >= 1)
		{
			if (retval == -1)
				error("select crash");
			_desc_ready = 1;
			setTempFD(temp_fd);
			index = findAvailableWorker(workers);
			workers.find(index)->second->setServer(this);
			workers.find(index)->second->setIt(0);
			workers.find(index)->second->setStatus(false);
			workers.find(index)->second->setSocket(_max_sd);
			while (!workers.find(index)->second->getStatus())
				;
			workers.find(index)->second->setIt(0);
		}
		else
			log("\e[1;96m[IDLING]\e[0m");
	}
}

t_loc *Server::findInLoc(string s, t_loc *root)
{
	for (list<t_loc *>::iterator it = root->childs.begin(); it != root->childs.end(); it++)
		if ((*it)->path == s)
			return (*it);
	return (NULL);
}

t_find Server::findAllLoc(string path)
{
	size_t pos;

	string split;

	t_loc *tmp = _root;

	t_find founded = {_root, "", _root->options.params.find("root")->second + "/"};

	while ((pos = path.find("/")) != string::npos)
	{
		if (path[0] == '/')
		{
			founded.path += "/";
			path.erase(0, 1);
			continue;
		}
		split = path.substr(0, pos);
		tmp = findInLoc(split, tmp);
		if (!tmp)
			return founded;
		founded.path += split + "/";
		founded.loc = tmp;
		if (tmp->options.params.count("root"))
			founded.access += tmp->options.params.find("root")->second;
		else
			founded.access += split + "/";
		path.erase(0, pos + 1);
	}
	tmp = findInLoc(path, tmp);
	if (!tmp)
	{
		founded.access += path;

		return founded;
	}
	founded.path += path;
	founded.loc = +tmp;
	if (tmp->options.params.count("root"))
	{

		founded.access += tmp->options.params.find("root")->second;
	}
	else
	{

		founded.access += path;
	}

	return founded;
}

t_file Server::getFile(string path)
{
	t_file file = {"", -1};
	t_find founded = findAllLoc(path);
	int fd;

	size_t pos = founded.path.size();
	if (pos == 1)
		pos--;

	string a_i = path;
	path.erase(0, pos);

	path = founded.loc->options.params.find("root")->second + path;

	if ((fd = open(path.c_str(), O_RDONLY | O_NONBLOCK | O_DIRECTORY)) != -1)
	{
		if (!_auto_index)
		{
			if (!founded.loc->options.params.count("index"))
			{
				std::cout << "Directory !" << endl;
				close(fd);
				return file;
			}
			else
				path += founded.loc->options.params.find("index")->second;
		}
		else
		{
			file.content = "<html> <head><title>Index of " + a_i + "</title></head> <body bgcolor=\"white\"> <h1>Index of " + a_i + "</h1><hr><pre>\n";

			DIR *dir;
			DIR *dir_tmp;

			struct dirent *ent;
			size_t size;
			size_t size_2;
			if ((dir_tmp = opendir(path.c_str())) != NULL)
			{
				size_2 = 0;
				while ((ent = readdir(dir_tmp)) != NULL)
				{
					struct stat buf;
					bzero(&buf, sizeof(buf));
					stat(string(path + "/" + string(ent->d_name)).c_str(), &buf);
					string size_file = to_string(buf.st_size);
					if (size_2 < size_file.size())
						size_2 = size_file.size();
				}
			}
			if ((dir = opendir(path.c_str())) != NULL)
			{
				size = file.content.size();
				while ((ent = readdir(dir)) != NULL)
				{
					size = string(ent->d_name).size();
					if (ent->d_type == DT_DIR)
					{
						size++;
						file.content += "<a href=\"" + string(ent->d_name) + "/\">" + string(ent->d_name) + "/</a>";
					}
					else
						file.content += "<a href=\"" + string(ent->d_name) + "\">" + string(ent->d_name) + "</a>";
					if (string(ent->d_name) != ".." && string(ent->d_name) != ".")
					{
						while (size++ < 51)
							file.content += " ";
						struct stat buf;
						bzero(&buf, sizeof(buf));
						stat(string(path + "/" + string(ent->d_name)).c_str(), &buf);

						struct timeval tv;
						time_t t;
						struct tm *info;
						char buffer[64];

						gettimeofday(&tv, NULL);
						t = tv.tv_sec;

						info = localtime(&(buf.st_mtime));
						strftime(buffer, sizeof(buffer), "%d-%b-%Y %H:%M", info);
						file.content += buffer;
						size_t i = 0;
						while (i++ < 20 - size_2)
							file.content += " ";
						string size_file = to_string(buf.st_size);
						i = size_file.size();
						while (i++ != size_2)
							file.content += " ";
						file.content += size_file;
					}
					file.content += "\n";
				}

				closedir(dir);
			}
			file.content += "</pre><hr></body> </html> ";
			file.size = file.content.size();
			close(fd);
			return file;
		}
		close(fd);
	}
	if ((fd = open(path.c_str(), O_RDONLY | O_NONBLOCK)) == -1)
		return file;
	struct stat buf;
	bzero(&buf, sizeof(buf));
	stat(path.c_str(), &buf);

	int size = buf.st_size;
	char c;
	int ret = 0;
	int readed;

	while (ret != size)
	{
		readed = read(fd, &c, 1);
		if (readed == -1)
		{
			close(fd);
			std::cout << "Error while reading" << endl;
			return file;
		}
		ret += readed;
		file.content.push_back(c);
	}
	file.size = file.content.size();
	close(fd);
	return file;
}

void Server::handle_request(Client &client)
{
	string type[5] = {"GET", "POST", "HEAD", "PUT", "DELETE"};
	string (Server::*command[])(map<string, string>, Client &) = {&Server::GET, &Server::POST, &Server::HEAD, &Server::PUT, &Server::DELETE};
	string buffer = client.getContent();
	int n;

	Headers request;
	request += buffer;
	map<string, string> p_request = request.last();
	string response, req_type = p_request.find("Request-Type")->second;

	// A mettre si debug
	// cout << buffer << endl;
	for (size_t i = 0; i <= 4; i++)
	{
		if (!strncmp(req_type.c_str(), type[i].c_str(), req_type.size()))
		{
			if (check_methods(findAllLoc(p_request.find("Location")->second).loc, req_type, p_request.find("Location")->second))
				response = (this->*command[i])(p_request, client);
			else
				response = SEND_ERROR(STATUS_METHOD_NOT_ALLOWED, "Method Not Allowed");
			break;
		}
		else if (i == 4)
		{
			response = SEND_ERROR(STATUS_BAD_REQUEST, "Bad Request");
			break;
		}
	}
	// A mettre si debug
	// cout << response << endl;

	n = 1;
	while (n > 0)
	{

		size_t szie = response.size();
		if (szie > 100000)
			szie = 100000;

		n = send(client.getSocket(), response.c_str(), szie, 0);

		response.erase(0, n);

		if (!response.size())
			break;
	}
	if (n < 0)
		error("Can't send the response");
}

void Server::error(const char *s)
{
	perror(s);
	exit(1);
}

// POST method
string Server::POST(map<string, string> header, Client &client)
{
	Headers tmp;

	string
		resp,
		content = header.find("Content")->second.c_str();
	int nb_print = 0;
	char buffer[65535];
	ofstream ofile;

	string loc = header.find("Location")->second;

	log("\e[1;93m[POST -> " + header.find("Location")->second + "]\e[0m");
	string extension;
	for (string::reverse_iterator it = loc.rbegin(); it != loc.rend(); it++)
	{
		extension.insert(extension.begin(), *it);
		if (*it == '.')
			break;
	}
	if (header.find("Content-Length")->second == "0")
		return (tmp.return_response_header(STATUS_NO_CONTENT, tmp, 0));
	client.setPath(loc);
	t_find founded = findAllLoc(header.find("Location")->second);
	if (_extensions.count(extension) && _extensions.find(extension)->second.count("cgi_path"))
	{
		CGI *cgi = new CGI();
		string loc_cgi = founded.access;
		string content_cgi = cgi->getContentFromCGI(header, (char *)(*_extensions.find(extension)->second.find("cgi_path")->second.begin()).c_str(), loc_cgi);
		resp = tmp.return_response_header(STATUS_OK, tmp, content_cgi.size());
		if (header.find("Location")->second != "/directory/youpi.bla")
		{
			resp = tmp.return_response_header(STATUS_OK, tmp, content_cgi.size());
			size_t pos = resp.find("\r\n\r\n");
			resp = resp.substr(0, pos + 2);
			resp += content_cgi;
		}
		else
		{
			content_cgi.erase(0, 16);
			resp = tmp.return_response_header(STATUS_OK, tmp, 0);
			size_t pos = resp.find("\r\n\r\n");
			resp = resp.substr(0, pos + 2);
			resp += content_cgi;
		}
		delete cgi;
		return resp;
	}

	// Check if the content is chunked
	if (header.count("Transfer-Encoding") && !strncmp(header.find("Transfer-Encoding")->second.c_str(), "chunked", 8))
	{
		readPerChunks(client, "POST", header);

		resp = tmp.return_response_header(STATUS_OK, tmp, 0);
		return resp;
	}

	t_file file = getFile(header.find("Location")->second);
	if (header.count("Content") && !header.find("Content")->second.size())
		resp = SEND_ERROR(STATUS_NO_CONTENT, "No Content");

	// Open the file and print content in
	ofile.open(founded.access.c_str(), ios::app);
	int maxSize = -1;
	if (founded.loc->options.params.count("maxBody"))
		maxSize = atoi(founded.loc->options.params.find("maxBody")->second.c_str());
	while (nb_print < atoi(header.find("Content-Length")->second.c_str()))
	{
		bzero(buffer, 65535);
		if (maxSize >= 0 && static_cast<int>(content.size()) > maxSize)
		{
			content = content.substr(0, maxSize);
			ofile << content;
			break;
		}
		ofile << content;
		nb_print += content.size();
		content.clear();
		// int rec;

		recv(client.getSocket(), buffer, 65535, 0);

		// nb_print += rec;
		content = buffer;
	}
	cout << content << endl;
	ofile.close();
	resp = tmp.return_response_header(200, tmp, 0);
	return resp;
}

string Server::DELETE(map<string, string> header, Client &client)
{
	(void)client;
	string content;
	Headers tmp;
	log("\e[1;93m[DELETE -> " + header.find("Location")->second + "]\e[0m");

	string file = findAllLoc(header.find("Location")->second).access;
	remove(file.c_str());

	content = tmp.return_response_header(STATUS_OK, tmp, 0);
	return (content);
}

// PUT method (create new file or trunk existent content)
string Server::PUT(map<string, string> header, Client &client)
{
	(void)client;

	log("\e[1;93m[PUT -> " + header.find("Location")->second + "]\e[0m");
	Headers tmp;
	ofstream ofile;
	string resp, content, path = header.find("Location")->second;
	string loc = header.find("Location")->second;

	string extension;
	for (string::reverse_iterator it = loc.rbegin(); it != loc.rend(); it++)
	{
		extension.insert(extension.begin(), *it);
		if (*it == '.')
			break;
	}
	t_find founded = findAllLoc(header.find("Location")->second);
	if (_extensions.count(extension) && _extensions.find(extension)->second.count("cgi_path"))
	{

		CGI *cgi = new CGI();
		string content_cgi = cgi->getContentFromCGI(header, (char *)(*_extensions.find(extension)->second.find("cgi_path")->second.begin()).c_str(), founded.loc->options.params.find("root")->second + loc);
		if (header.find("Location")->second != "/directory/youpi.bla")
		{
			resp = tmp.return_response_header(STATUS_OK, tmp, content_cgi.size());
			size_t pos = resp.find("\r\n\r\n");
			resp = resp.substr(0, pos + 2);
			resp += content_cgi;
		}
		else
		{
			content_cgi.erase(0, 16);
			resp = tmp.return_response_header(STATUS_OK, tmp, 0);
			size_t pos = resp.find("\r\n\r\n");
			resp = resp.substr(0, pos + 2);
			resp += content_cgi;
		}
		delete cgi;
		return resp;
	}
	ofile.open(founded.path.c_str(), ios::app);

	if (header.count("Transfer-Enconding") && !strncmp(header.find("Transfer-Encoding")->second.c_str(), "chunked", 8))
	{
		readPerChunks(client, "PUT", header);
		resp = tmp.return_response_header(200, tmp, content.size());
		return resp;
	}
	else
		content = header.find("Content")->second;
	ofile.close();

	const char *content_char = content.c_str();
	const char *to_print = content_char;

	t_file file = getFile(header.find("Location")->second);
	if (header.count("Content") && !header.find("Content")->second.size())
		resp = SEND_ERROR(STATUS_NO_CONTENT, "No Content");

	int nb_prints, fd = open(founded.access.c_str(), O_TRUNC | O_CREAT | O_WRONLY | O_NONBLOCK, 0777), maxSize = INT_MAX;
	size_t remaining_characters = content.size(), count = 0;
	if (founded.loc->options.params.count("maxBody"))
		maxSize = atoi(founded.loc->options.params.find("maxBody")->second.c_str());
	while (count < remaining_characters)
	{
		if (65535 < count)
			nb_prints = 65535;
		else
		{
			to_print = content_char + count;
			nb_prints = strlen(content_char);
		}
		if (maxSize >= 0 && maxSize < nb_prints)
		{
			write(fd, to_print, maxSize);
			break;
		}
		count += write(fd, to_print, nb_prints);
	}

	close(fd);

	resp = tmp.return_response_header(200, tmp, content.size());
	resp += content;
	return (resp);
}

// A ameliorer plus tard
string Server::HEAD(map<string, string> header, Client &client)
{
	(void)client;
	Headers tmp;
	string resp;
	t_file file = getFile(header.find("Location")->second);

	resp = tmp.return_response_header(200, tmp, 0);
	resp += "Location: http://localhost:8080/\r\n\r\n";
	return resp;
}

//A AMELIORER plus tard
string Server::GET(map<string, string> header, Client &client)
{
	(void)client;
	log("\e[1;93m[GET -> " + header.find("Location")->second + "]\e[0m");

	t_file file = getFile(header.find("Location")->second);
	Headers tmp;
	string resp;

	string extension;
	string loc = header.find("Location")->second;
	for (string::reverse_iterator it = loc.rbegin(); it != loc.rend(); it++)
	{
		extension.insert(extension.begin(), *it);
		if (*it == '.')
			break;
	}

	if (_extensions.count(extension) && _extensions.find(extension)->second.count("cgi_path"))
	{
		t_find founded = findAllLoc(header.find("Location")->second);

		CGI *cgi = new CGI();
		string loc_cgi = founded.access;
		string content_cgi = cgi->getContentFromCGI(header, (char *)(*_extensions.find(extension)->second.find("cgi_path")->second.begin()).c_str(), loc_cgi);
		if (header.find("Location")->second != "/directory/youpi.bla")
		{
			resp = tmp.return_response_header(STATUS_OK, tmp, content_cgi.size());
			size_t pos = resp.find("\r\n\r\n");
			resp = resp.substr(0, pos + 2);
			resp += content_cgi;
		}
		else
		{
			content_cgi.erase(0, 16);
			resp = tmp.return_response_header(STATUS_OK, tmp, 0);
			size_t pos = resp.find("\r\n\r\n");
			resp = resp.substr(0, pos + 2);
			resp += content_cgi;
		}
		delete cgi;
		return resp;
	}

	if (file.size == -1)
		resp = SEND_ERROR(STATUS_NOT_FOUND, "Not Found");
	else if (header.count("coffee"))
		resp = SEND_ERROR(STATUS_TEAPOT, "I'm a teapot");
	else
	{
		if (_mimes.count(extension))
			resp = tmp.return_response_header(200, tmp, file.size, _mimes.find(extension)->second);
		else
			resp = tmp.return_response_header(200, tmp, file.size);
		resp += file.content;
	}

	return resp;
}

// SOUCIS: si on  location /foo/ et location /foo/bar/ dans le .conf et aue l'url est /foo/bar/, il va chopper /foo/
// il faut donc le comparer avec tout et prendre celui avec le + de char correspondant

// Generate a default error page -- TODO: add conf errors page
string Server::SEND_ERROR(int status, const char *msg)
{
	t_file file;
	string resp;
	Headers tmp;

	char c;
	size_t pos;

	log("\e[1;31m[Method Not Allowed]\e[0;0m");

	string page;
	if (_error_pages.count(status))
		page = _error_pages.find(status)->second;
	else
		page = "default/error.html";
	int fd = open(page.c_str(), O_RDONLY | O_NONBLOCK);
	while (read(fd, &c, 1))
		file.content.push_back(c);
	while ((pos = file.content.find("_CODE_")) != file.content.npos)
		file.content.replace(file.content.find("_CODE_"), 6, to_string(status));
	while ((pos = file.content.find("_MSG_")) != file.content.npos)
		file.content.replace(file.content.find("_MSG_"), 5, msg);
	file.size = file.content.size();
	resp = tmp.return_response_header(status, tmp, file.size);
	resp += file.content;
	close(fd);
	return resp;
}

void Server::log(string s)
{
	struct timeval tv;
	time_t t;
	struct tm *info;
	char buffer[64];
	pthread_mutex_lock(_logger);

	gettimeofday(&tv, NULL);
	t = tv.tv_sec;

	info = localtime(&t);
	strftime(buffer, sizeof(buffer), "[%c]", info);
	string name;
	if (_name != "default_server")
		name = _name;
	else
		name = "SERVER " + to_string(_id);
	std::cout << "\e[1;" << to_string(93 + _id) << "m[" << name << "::" << _port << "]\e[0m" << s << " \e[1;90m" << buffer << "\e[0m" << endl;

	pthread_mutex_unlock(_logger);
}

bool Server::check_methods(t_loc *root, string meth, string loc)
{
	string extension;
	vector<string> allowed;
	for (string::reverse_iterator it = loc.rbegin(); it != loc.rend(); it++)
	{
		extension.insert(extension.begin(), *it);
		if (*it == '.')
			break;
	}
	if (_extensions.count(extension) && _extensions.find(extension)->second.count("allow_methods"))
		allowed = _extensions.find(extension)->second.find("allow_methods")->second;
	else
		allowed = root->options.methods;
	for (vector<string>::iterator it = allowed.begin(); it != allowed.end(); it++)
		if (*it == meth)
			return true;
	return false;
}

int Server::findAvailableWorker(map<int, Worker *> &workers)
{
	size_t i = 0;
	while (1)
	{
		i = 0;
		while (!workers.find(i)->second->getStatus() && i < workers.size() - 1)
			i++;
		if (workers.find(i)->second->getStatus())
			break;
	}
	return (i);
}

string Server::readPerChunks(Client &client, string method, map<string, string> header)
{
	string content, temp;

	size_t pos = client.getContent().find("\r\n\r\n", 0);
	temp = &client.getContent().at(pos + 4);
	ofstream ofile;

	string path_file = findAllLoc(header.find("Location")->second).path;
	if (!strcmp("PUT", method.c_str()))
		ofile.open(path_file.c_str(), ios::out);
	else
		ofile.open(path_file.c_str());
	t_find file = findAllLoc(path_file);

	// Get the length of the chunk
	string length_char;
	int i = 0, length = 0;
	while (temp.at(i) != '\r' && temp.at(i) != '\n')
		length_char += temp.at(i++);

	// Remove hexadecimal characters and \r\n + transform hexadecimal length to decimal
	temp = &temp.at(i + 2);
	stringstream stream;
	stream << hex << length_char;
	stream >> length;

	if (!length)
		return content;

	// Append existent content from worker's recv
	for (int j = 0; j < length; j++)
		content += temp.at(j);
	temp = &temp.at(length + 2);
	ofile << content;
	char buff[length + 1];

	while (true)
	{
		char buf[65535];
		bzero(buf, 65535);
		length_char.clear();
		i = 0;
		while (temp.at(i) != '\r' && temp.at(i) != '\n')
			length_char += temp.at(i++);
		temp = &temp.at(i + 2);

		stringstream stream;
		stream << hex << length_char;
		stream >> length;
		if (!length)
			break;
		int length_copy = 0;
		if (length > static_cast<int>(temp.size()))
		{
			ofile << temp;
			length_copy = temp.size();
		}
		else
		{
			bzero(buff, length + 1);
			temp.copy(buff, length, 0);
			temp = &temp.at(length + 2);
			ofile << buff;
			continue;
		}

		int nbytes_read = 0;
		usleep(50);
		do
		{
			nbytes_read += recv(client.getSocket(), &buf[strlen(buf)], 65000 - strlen(buf), 0);
			if (strstr(buf, "0\r\n\r\n"))
				break;
		} while (65000 - strlen(buf) > 1);

		temp.clear();
		temp = buf;
		bzero(buff, length + 1);
		temp.copy(buff, length - length_copy, 0);
		ofile << buff;
		temp = &temp.at(length - length_copy + 2);
		content.clear();
	}
	content.clear();
	ofile.close();
	return (content);
}
