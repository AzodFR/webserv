#include "Headers.hpp"

Headers::Headers()
{
}

Headers::~Headers()
{
}

string::iterator get_ite(string tmp, size_t pos)
{
	return tmp.begin() + pos;
}
//Request-Type nom de la key pour le POST, GET etc
//Location == /foo/bar
//HTTP-Ver version HTTP
void Headers::operator+=(string send)
{
	istringstream to_parse(send);
	map<string, string> head;
	string line;
	string content;
	int i = 0;
	while (getline(to_parse, line))
	{
		cout << i++ << endl;
		if (line == "\r")
		{

			while(getline(to_parse, line))
				content += line + "\n";
			head.insert(make_pair("Content", content));
			cout << content.find("\r\n\r\n") << endl;
			break;
		}
		line[line.size() - 1] = '\0';
		if (head.size() == 0)
		{
			size_t pos = line.find('/');
			string type = line.substr(0, pos - 1);
			head.insert(make_pair("Request-Type", type));
			line.erase(0, pos);
			pos = line.find("HTTP");
			string loc = line.substr(0, pos - 1);
			cout << "location: " << loc << endl;
			if ((pos = loc.find("?")) != string::npos)
			{

				string token = loc.substr(0, pos);
				head.insert(make_pair("Location", trim_whitespace(token)));
				loc.erase(0, pos + 1);
				head.insert(make_pair("Variables", trim_whitespace(loc)));
			}
			else
				head.insert(make_pair("Location", trim_whitespace(loc)));
			line.erase(0, pos);
			pos = line.find("\r\n");
			string ver = line.substr(0, pos - 1);
			head.insert(make_pair("HTTP-Ver", ver));
		}
		else
		{
			string index(line, 0, line.find_first_of(':'));
			string content(line, line.find_first_of(':') + 2, line.size() - 2);
			head.insert(make_pair(index, content));
		}
	}
	if (!head.count("Content-Length"))
		head.insert(make_pair("Content-Length", "0"));
	_headers.insert(make_pair(_headers.size(), head));

//	exit(1);
}

int Headers::check(map<string, string> actual)
{
	string ver = actual.find("HTTP-Ver")->second;
	if (strcmp(actual.find("HTTP-Ver")->second.c_str(), "1.1"))
		return 505; // 505 bad version
	if (actual.find("Location")->second.size() > 100000000)
		return 414; // 414 too long uri
	if (!actual.count("Host"))
		return 400; // 400 no host
	if (!strcmp(actual.find("Request-Type")->second.c_str(), "POST") && !actual.find("Content")->second.size())
		return 204;
	return 200;
}

string Headers::return_response_header(int status, Headers header, size_t size_content)
{
	(void)status;
	(void)header;
	string response;

	// Parse status code
	switch (status)
	{
	case STATUS_OK:
		response = "HTTP/1.1 200 OK\r\n";
		break;
	case STATUS_NO_CONTENT:
		response = "HTTP/1.1 204 No Content\r\n";
		break;
	case STATUS_NOT_FOUND:
		response = "HTTP/1.1 404 Not Found\r\n";
		break;
	case STATUS_METHOD_NOT_ALLOWED:
		response = "HTTP/1.1 405 Method Not Allowed\r\n";
		break;
	case STATUS_TEAPOT:
		response = "HTTP/1.1 418 I'm a teapot\r\n";
		break;
	case STATUS_HEAD:
		response = "HTTP/1.1 200 OK\r\n";
		break;
	case STATUS_URI_TOO_LONG:
		response = "HTTP/1.1 414 URI Too Long\r\n";
		break;
	case STATUS_BAD_REQUEST:
		response = "HTTP/1.1 400 Bad Request\r\n";
		break;
	default:
		break;
	}
	// Parse date
	struct timeval tv;
	time_t t;
	struct tm *info;
	char buffer[64];

	gettimeofday(&tv, NULL);
	t = tv.tv_sec;

	info = localtime(&t);
	strftime(buffer, sizeof buffer, " ", info);
	response += "Date:";
	response += buffer;
	strftime(buffer, sizeof buffer, "%p %I:%M GMT +2.\r\n", info);
	response += asctime(info);
	response += "Server: webserv\r\n";
	// If method is GET/HEAD, its gonna fill this line
	if (status == STATUS_NO_CONTENT)
		response += "Content-Length: 0\r\n";
	else if (size_content > 0)
		response += "Content-Length: " + std::to_string(size_content) + "\r\n";
	response += "Content-Language: fr-FR\r\n";
	//response += "Cookie: user-color = #eb4034\r\n";
	if (status != STATUS_HEAD)
		response += "\r\n";
	// cout << response;
	return (response);
}

string Headers::return_response_header(int status, Headers header, size_t size_content, string type)
{
	(void)status;
	(void)header;
	string response;

	// Parse status code
	switch (status)
	{
	case STATUS_OK:
		response = "HTTP/1.1 200 OK\r\n";
		break;
	case STATUS_NO_CONTENT:
		response = "HTTP/1.1 204 No Content\r\n";
		break;
	case STATUS_NOT_FOUND:
		response = "HTTP/1.1 404 Not Found\r\n";
		break;
	case STATUS_METHOD_NOT_ALLOWED:
		response = "HTTP/1.1 405 Method Not Allowed\r\n";
		break;
	case STATUS_TEAPOT:
		response = "HTTP/1.1 418 I'm a teapot\r\n";
		break;
	case STATUS_HEAD:
		response = "HTTP/1.1 200 OK\r\n";
		break;
	case STATUS_URI_TOO_LONG:
		response = "HTTP/1.1 414 URI Too Long\r\n";
		break;
	case STATUS_BAD_REQUEST:
		response = "HTTP/1.1 400 Bad Request\r\n";
		break;
	default:
		break;
	}

	// Parse date
	struct timeval tv;
	time_t t;
	struct tm *info;
	char buffer[64];

	gettimeofday(&tv, NULL);
	t = tv.tv_sec;

	info = localtime(&t);
	strftime(buffer, sizeof buffer, " ", info);
	response += "Date:";
	response += buffer;
	strftime(buffer, sizeof buffer, "%p %I:%M GMT +2.\r\n", info);
	response += asctime(info);
	response += "Server: webserv\r\n";
	// If method is GET/HEAD, its gonna fill this line
	if (status == STATUS_NO_CONTENT)
		response += "Content-Length: 0\r\n";
	else if (size_content > 0)
		response += "Content-Length: " + to_string(size_content) + "\r\n";
	response += "Content-Language: fr-FR\r\n";
	response += "Content-Type: " + type + "\r\n";
	if (status != STATUS_HEAD)
		response += "\r\n";
	// cout << response;
	return (response);
}