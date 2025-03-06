#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cstring>

#include "helpers.h"

using namespace std;

// functie care primeste fix len bytes
int recv_all(int sockfd, void *buffer, int len) {

	int bytes_received = 0;
	int bytes_remaining = len;
	char *buff = (char *)buffer;

  	while (bytes_remaining > 0) {
		int rc = recv(sockfd, buff + bytes_received, bytes_remaining, 0);
		DIE(rc == -1, "recv() failed");

		if (!rc)
			return 0;

		bytes_received += rc;
		bytes_remaining -= rc;
  	}

	return bytes_received;
}

// functie care trimite un anumit nr de bytes, cat lungimea mesajului
int send_all(int sockfd, void *buffer, int len) {
	int bytes_sent = 0;
	int bytes_remaining = len;
	char *buff = (char *)buffer;

	while (bytes_remaining > 0) {
		int rc = send(sockfd, buff + bytes_sent, bytes_remaining, 0);
		DIE(rc == -1, "sent() failed");

		bytes_sent += rc;
		bytes_remaining -= rc;
	}

  	return bytes_sent;
}

// verific daca exista caracterul si de cate ori apare
int containsChar(const string& str, char c) {
    int count = 0;
    size_t pos = str.find(c);
    while (pos != string::npos) {
        count++;
        pos = str.find(c, pos + 1);
    }
    return count;
}

// vad daca un substring e in string
bool containsSubstr(const string& str, const string& substr) {
    return str.find(substr) != string::npos;
}

// vad daca un string e intr-un vector de string-uri
bool stringInVector(const string& str, const vector<string>& vec) {
    return find(vec.begin(), vec.end(), str) != vec.end();
}

bool matchesTopic(const string& topic, vector<string>& topics) {

    // caut in toate topicurile daca exista cea dorita
    for (string& a : topics) {

        // nu avem wildcard
        if (containsChar(a, '*') == 0 && containsChar(a, '+') == 0) {
            if (topic == a) {
                return true;
            }

        // avem unul sau mai multe * si atat
        } else if (containsChar(a, '*') > 0 && containsChar(a, '+') == 0) {
            size_t pos = 0;
            size_t lastPos = 0;
            bool matched = true;
            // caut pozitia la care se gaseste *
            while ((pos = a.find('*', lastPos)) != string::npos) {

                // iau sufixul si vad daca se regaseste in topic
                string part = a.substr(lastPos, pos - lastPos);
                if (topic.find(part) == string::npos) {
                    matched = false;
                    break;
                }

                // continui de la pozitia la care am gasit * plus 1
                lastPos = pos + 1;
            }

            // topicul s-a potrivit
            if (matched) {
                string suffix = a.substr(lastPos);

                // topicul s-a potrivit complet
                if (topic.rfind(suffix) == topic.length() - suffix.length()) {
                    return true;
                }
            }
        }

        // avem + dar nu si *
        else if (containsChar(a, '*') == 0 && containsChar(a, '+') > 0) {
            size_t count = containsChar(a, '+');
            size_t pos;
            string prefix, suffix;
            string rest = a;

            // pana cand nu mai sunt plusuri
            while (count != 0) {
                pos = rest.find('+');
                prefix = rest.substr(0, pos);
                suffix = rest.substr(pos + 1);
                rest = suffix;

                // vedem daca prefixul e continut
                if (!containsSubstr(topic, prefix)) {
                    return false;
                }
                count--;
            }

            // daca sufixul final e continut si + a fost inlocuit de un singur cuvant
            if (suffix == "" || topic.find(suffix) != string::npos) {
                if (containsChar(topic, '/') == containsChar(a, '/')) {
                    return true;
                }
            }

        // avem si + si *
        } else {
			size_t count = containsChar(a, '*');
            size_t pos1, pos2, pos3;
            string prefix, suffix;
            string rest = a;

            // cat timp mai sunt *, verific daca in prefix e un + sau mai multe
            // le inlocuiesc, apoi vad si in sufix la fel ca mai sus la cazul cu +
            while (count != 0) {
                pos1 = rest.find('*');
                prefix = rest.substr(0, pos1);
                suffix = rest.substr(pos1 + 1);
                rest = suffix;

                // verific de + uri in prefix si inlocuiesc
                if (containsChar(prefix, '+')) {
                    size_t count1 = containsChar(prefix, '+');
                    string prefix1, suffix1;
                    string rest1 = prefix;
                    while (count1 != 0) {
                        pos2 = rest1.find('+');
                        prefix1 = rest1.substr(0, pos2);
                        suffix1 = rest1.substr(pos2 + 1);
                        rest1 = suffix1;

                        if (!containsSubstr(topic, prefix1)) {
                            return false;
                        }
                        count1--;
                    }
                    if (!containsSubstr(topic, suffix1)) {
                        return false;
                    }

                // prefixul fara wildcarduri nu e continut
                } else if (!containsSubstr(topic, prefix) && prefix != "") {
                    return false;
                }

                // verific sufixul de + uri
                if (containsChar(suffix, '+')) {
                    size_t count2 = containsChar(suffix, '+');
                    string prefix2, suffix2;
                    string rest2 = suffix;
                    while (count2 != 0) {
                        pos2 = rest2.find('+');
                        prefix2 = rest2.substr(0, pos2);
                        suffix2 = rest2.substr(pos2 + 1);
                        rest2 = suffix2;

                        if (!containsSubstr(topic, prefix2)) {
                            return false;
                        }
                        count2--;
                    }
                    if (!containsSubstr(topic, suffix2) && suffix2 != "") {
                        return false;
                    }

                    pos3 = topic.find(prefix2);
                    rest2 = topic.substr(pos3);
                    if (containsChar(suffix, '/') != containsChar(rest2, '/')) {
                        return false;
                    }
                    suffix = suffix2;
                }
                count--;
            }

            // daca sufixul e continut, se potriveste
            if (suffix == "" || topic.find(suffix) != string::npos) {
                return true;
            }
		}
    }
    return false;
}

// cazul in care primesc un mesaj de la udp
void udp_case(unordered_map<string, vector<string>> client_topics, int udp, unordered_map<string, int> id_socket) {
    char buff[1552];
    memset(buff, 0, sizeof(buff));

    // citesc in buffer datele primite          
    int rc = (int) recv(udp, buff, sizeof(buff), 0);
    DIE(rc < 0, "udp receive");

    // creez un mesaj pe care sa-l trimit, ii retin lungimea
    tcp_msg *msg = new tcp_msg;
    msg->len = rc;
    msg->data = new char[rc];
    memcpy(msg->data, buff, rc);

    // iau numele topicului
    char *topic_name = new char[51];
    memset(topic_name, 0, 51);
    memcpy(topic_name, (struct udp_msg *) buff, 50);

    // iterez prin toti clientii si topicurile la care sunt abonati ca sa vad daca le dau mesajul
    for (auto it = client_topics.begin(); it != client_topics.end(); ++it) {
	    string client = it->first;
		vector<string> topics = it->second;

        // sunt abonati la topic
        if (matchesTopic(topic_name, topics)) {
			int client_socket = id_socket[client];

            // sunt online
			if (client_socket >= 0) {
				send_all(client_socket, &msg->len, sizeof(msg->len));
                send_all(client_socket, msg->data, msg->len);
			}
		}
    }   
}

// cazul in care se conecteaza un client tcp
void tcp_case(int tcp, unordered_map<string, int>& id_socket, unordered_map<int,
              string>& socket_id, fd_set& read_fds, int& max_fd) {

    // acceptam conexiunea cu clientul tcp
    auto *client_address = new sockaddr_in;
    socklen_t client_length = sizeof(struct sockaddr_in);
    int client = accept(tcp, (struct sockaddr *) client_address, &client_length);
    DIE(client < 0, "accept");
    
    // mesaj in care primesc id-ul noului client
    auto *msg = new command_message;
    memset(msg, 0, sizeof(command_message));
    int rc = (int) recv(client, msg, sizeof(command_message), 0);
    DIE(rc < 0, "tcp receive");
    
    // verific daca id-ul clientului e deja folosit
    char *client_id = msg->topic;
    if (id_socket.find(client_id) != id_socket.end()) {
        if (id_socket[client_id] != -1) {
            close(client);
            cout << "Client " << client_id << " already connected." << endl;
            return;
        }
    }
    
    FD_SET(client, &read_fds);
    max_fd = max(client, max_fd);

    // adaug id-ul in map-ul de clienti online si de id-uri
    if (id_socket.find(client_id) != id_socket.end()) {
        id_socket[client_id] = client;
        socket_id.insert(make_pair(client, client_id));
    } else {
        id_socket.insert(make_pair(client_id, client));
        socket_id.insert(make_pair(client, client_id));
    }
    
    // afisez noul client conectat
    cout << "New client " << client_id << " connected from ";
    auto *string_address = inet_ntoa(client_address->sin_addr);
    auto string_port = ntohs(client_address->sin_port);
    cout << string_address << ":" << string_port << endl;
}

// s-a primit o comanda de subscribe
void subscribe(command_message *subscribe_message, unordered_map<string, vector<string>>& client_topics,
                unordered_map<int, string>& socket_id, int i) {

    // daca e prima abonare creez lista de abonari si adaug topicul
    if (client_topics.find(socket_id[i]) == client_topics.end()) {
        vector<string> topics_by_client;
		topics_by_client.push_back(subscribe_message->topic);
		client_topics.insert(make_pair(socket_id[i], topics_by_client));
    
    // nu e prima abonare, adaug noul topic daca nu e deja
	} else {
		auto topics_by_client = client_topics[socket_id[i]];
        auto is_topic = find(topics_by_client.begin(), topics_by_client.end(), subscribe_message->topic);

        if (is_topic == topics_by_client.end()) {
            topics_by_client.push_back(subscribe_message->topic);
            client_topics[socket_id[i]] = topics_by_client;
        }
    }
}

// s-a primit o comanda de unsubscribe
void unsubscribe(command_message *unsubscribe_message, unordered_map<string, vector<string>>& client_topics,
                unordered_map<int, string>& socket_id, int i) {
    
    // daca topicul exista in lista clinetului il scot
	vector<string> topics_by_client = client_topics[socket_id[i]];
    auto is_topic = find(topics_by_client.begin(), topics_by_client.end(), unsubscribe_message->topic);

    if (is_topic != topics_by_client.end()) {
		topics_by_client.erase(remove(topics_by_client.begin(), topics_by_client.end(), unsubscribe_message->topic), topics_by_client.end());
		client_topics[socket_id[i]] = topics_by_client;
	}
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
		cerr << "wrong command" << endl;
		exit(0);
	}

    // clientii care sunt offline au -1
    unordered_map<string, int> online;
    // socket-ul si id-ul corespunzator
	unordered_map<int, string> socket_id;
    // clientii si topicurile la care sunt abonati
	unordered_map<string, vector<string>> client_topics;

    // iau portul
	uint16_t port;
	int rc = sscanf(argv[1], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

    // deschid socket ptr tcp
	int tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp < 0, "socket_tcp");

	struct sockaddr_in tcp_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);

    // initializez adresa de primire
	memset(&tcp_addr, 0, socket_len);
	tcp_addr.sin_family = AF_INET;
	tcp_addr.sin_port = htons(port);

	int enable = 1;
	if (setsockopt(tcp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

    // leg socketul si adresa
	rc = bind(tcp, (const struct sockaddr *) &tcp_addr, sizeof(tcp_addr));
	DIE(rc < 0, "bind");

	rc = listen(tcp, 128);
	DIE(rc < 0, "listen");

    // la fel si pentru udp
	int udp = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(udp < 0, "socket_udp");

	struct sockaddr_in udp_addr;

	memset(&udp_addr, 0, socket_len);
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(port);

	enable = 1;
	if (setsockopt(udp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	rc = bind(udp, (const struct sockaddr *) &udp_addr, sizeof(udp_addr));
	DIE(rc < 0, "bind");

	// setez file descriptorii
    fd_set fds, temp_fds;
    FD_ZERO(&fds);
	FD_ZERO(&temp_fds);

	FD_SET(0, &fds);
	FD_SET(udp, &fds);
	FD_SET(tcp, &fds);

	int max_fd = tcp;

	rc = setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	DIE(rc < 0, "setvbuf");

    int ok = 1;

    for (;;) {
        temp_fds = fds;

        // s-a primit comanda de exit
        if (!ok) {
            break;
        }

        // multiplexare I/O
        rc = select(max_fd + 1, &temp_fds, nullptr, nullptr, nullptr);
        DIE(rc < 0, "select");

        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &temp_fds)) {

                // s-a primit o comanda de la stdin
                if (i == STDIN_FILENO) {
                    string command;
                    cin >> command;
                    if (command == "exit") {

                        // sterg fd urile active
                        for (int j = 1; j <= max_fd; j++) {
                            if (FD_ISSET(j, &fds)) {
                                FD_CLR(j, &fds);
                                close(j);
                            }
                        }
                        ok = 0;
                        break;
                    }
                    continue;

                // s-a primit un mesaj de la clientii udp
                } else if (i == udp) {
                    udp_case(client_topics, udp, online);
                    break;
                
                // cazul ptr tcp
                } else if (i == tcp) {

                    //dezactivez algoritmul lui nagle
                    int flag = 1;
                    rc = setsockopt(tcp, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
                    DIE(rc < 0, "nagle");
                    
                    tcp_case(tcp, online, socket_id, fds, max_fd);
                    continue;
                }

                // s-a primit un mesaj de subscribe/unsubscribe
                auto *subscribe_message = new command_message;
                memset(subscribe_message, 0, sizeof(struct command_message));
                rc = (int) recv(i, subscribe_message, sizeof(command_message), 0);

                // clientul s-a dconectat, nu s-au primit bytes
                DIE(rc < 0, "recv");
                if (rc == 0) {
                    FD_CLR(i, &fds);
                    FD_CLR(i, &temp_fds);
                    close(i);

                    // trec id-ul offline
                    online[socket_id[i]] = -1;
                    cout << "Client " << socket_id[i] << " disconnected." << endl;
                    socket_id.erase(i);
                    continue;
                }
                if (subscribe_message->type == 0) {
                    unsubscribe(subscribe_message, client_topics, socket_id, i);
                }
                if (subscribe_message->type == 1) {
                    subscribe(subscribe_message, client_topics, socket_id, i);
                }
            }
        }
    }
    close(udp);
    close(tcp);
    return 0;
}
