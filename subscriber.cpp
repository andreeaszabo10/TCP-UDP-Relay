#include <iostream>
#include <vector>
#include <iomanip>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <cctype>
#include <set>

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

		if (rc == 0) {
			return 0;
		}

		bytes_received = bytes_received + rc;
		bytes_remaining = bytes_remaining - rc;
  	}

	return bytes_received;
}

// functie care trimite fix len bytes
int send_all(int sockfd, void *buffer, int len) {
	int bytes_sent = 0;
	int bytes_remaining = len;
	char *buff = (char *)buffer;

	while (bytes_remaining > 0) {
		int rc = send(sockfd, buff + bytes_sent, bytes_remaining, 0);
		DIE(rc == -1, "sent() failed");

		bytes_sent = bytes_sent + rc;
		bytes_remaining = bytes_remaining - rc;
	}

  	return bytes_sent;
}

// functie care sparge un sir in cuvinte
vector<string> get_words(const string& input) {
	vector<string> words;
	size_t start = 0, end;
	while ((end = input.find(' ', start)) != string::npos) {
		words.push_back(input.substr(start, end - start));
		start = end + 1;
	}
	words.push_back(input.substr(start));
	return words;
}

// afisez mesaj pentru int
void for_int(struct udp_msg *message, char* buff) {
	uint8_t sign_byte;
	// iau byte-ul de semn si numarul
	memcpy(&sign_byte, buff + 51, sizeof(uint8_t));
				
	uint32_t number;
	memcpy(&number, buff + 51 + sizeof(uint8_t), sizeof(uint32_t));
	int number_ok;

	// creez numarul si adaug semnul
	if (sign_byte == 1) {
		number_ok = ntohl(number) * (-1);
	} else {
		number_ok = ntohl(number);
	}

	cout << message->topic << " - INT - " << number_ok << endl;
}

// afisez mesaj pentru short
void for_short(struct udp_msg *message, char* buff) {
	uint16_t number;
	memcpy(&number, buff + 51, sizeof(uint16_t));

	// gasesc numarul si il impart la 100
	float number_ok = ntohs(number);
	float result = number_ok / 100;

	cout << message->topic<< " - SHORT_REAL - " << fixed << setprecision(2) << result << endl;
}

// afisez mesaj pentru float
void for_float(struct udp_msg *message, char* buff) {

	// iau byte ul de semn, numarul fara virgula si puterea
	uint8_t sign_byte;
	memcpy(&sign_byte, buff + 51, sizeof(uint8_t));
	
	uint32_t number;
	memcpy(&number, buff + 51 + sizeof(uint8_t), sizeof(uint32_t));
	double number_ok = ntohl(number);
	
	uint8_t power;
	memcpy(&power, buff + 51 + sizeof(uint8_t) + sizeof(uint32_t), sizeof(uint8_t));

	// impart pentru a separa partea intreaga de fractionara
	for (int i = 0; i < power; i++) {
		number_ok = number_ok / 10;
	}

	// adaug semnul
	if (sign_byte == 1) {
		number_ok = number_ok * (-1);
	}

	cout << message->topic << " - FLOAT - " << fixed << setprecision(4) << number_ok << endl;
}

// afisez mesaj pentru string
void for_string(struct udp_msg *message, char* buff) {
	cout << message->topic << " - STRING - " << (buff + 51) << endl;
}

// vad ce tip de date am si afisez mesajul
void write_message(struct udp_msg *message, char* buff) {
	if (message->type == 0)
		for_int(message, buff);

	if (message->type == 1)
		for_short(message, buff);

	if (message->type == 2)
		for_float(message, buff);

	if (message->type == 3)
		for_string(message, buff);
}

void subscribe(int sock, vector<string> words) {
	char *topic = (char *) words[1].c_str();

	// creez mesajul de subscribe si l trimit catre server
	auto *subscribe_message = new command_message;
	memset(subscribe_message, 0, sizeof(struct command_message));
	memcpy(subscribe_message->topic, topic, strlen(topic));
	subscribe_message->type = 1;

	int rc = (int) send(sock, subscribe_message, sizeof(struct command_message), 0);
	DIE(rc < 0, "send");

	cout << "Subscribed to topic " << topic << endl;

}

void unsubscribe(int sock, vector<string> words) {
	char *topic = (char *) words[1].c_str();

	// creez mesajul de unsubscribe si l trimit catre server
	auto *unsubscribe_message = new command_message;
	memset(unsubscribe_message, 0, sizeof(struct command_message));
	memcpy(unsubscribe_message->topic, topic, strlen(topic));
	unsubscribe_message->type = 0;

	int rc = (int) send(sock, unsubscribe_message, sizeof(struct command_message), 0);
	DIE(rc < 0, "send");

	cout << "Unsubscribed from topic "<< topic << endl;
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		cerr << "wrong command" << endl;
		exit(0);
	}

	char *subscriber_id = new char[11];
	memset(subscriber_id, 0, 11);
	memcpy(subscriber_id, argv[1], strlen(argv[1]));

	set<string> topics;
	setvbuf(stdout, nullptr, _IONBF, BUFSIZ);

	// iau portul
	uint16_t port;
	int rc = sscanf(argv[3], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	// creez un socket si l conectez la server
	auto server = new sockaddr_in;
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sock < 0, "socket");

	int enable = 1;
	rc = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &enable, sizeof(int));
	DIE(rc < 0, "nagle");

	server->sin_family = AF_INET;
	server->sin_port = htons(port);

	rc = connect(sock, (struct sockaddr *) server, sizeof(struct sockaddr_in));
	DIE(rc < 0, "connect");

	// creez un nou client si anunt serverul
	auto *connect = new command_message;
	memset(connect, 0, sizeof(struct command_message));
	memcpy(connect->topic, subscriber_id, 11);
	connect->type = 2;
	rc = (int) send(sock, connect, sizeof(command_message), 0);
	DIE(rc < 0, "connect id");

	// setez file descriptorii
	fd_set fds, temp_fds;
	FD_ZERO(&fds);
	FD_ZERO(&temp_fds);
	FD_SET(0, &fds);
	FD_SET(sock, &fds);
	

	int max_fd = sock;
	for (;;) {
		temp_fds = fds;
		rc = select(max_fd + 1, &temp_fds, nullptr, nullptr, nullptr);
		DIE(rc < 0, "select");

		// s-a primit comanda de la stdin
		if (FD_ISSET(STDIN_FILENO, &temp_fds)) {
			string all_words;
			getline(cin, all_words);
			vector<string> words = get_words(all_words);
			string command = words[0];

			if (command == "exit") {
				break;
			}

			// comanda nu e corecta
			if (command == "subscribe" || command == "unsubscribe") {
				if (words.size() < 2) {
					continue;
				}
				char *topic = (char *) words[1].c_str();
				if (strlen(topic) > 50) {
					continue;
				}
			}

			if (command == "subscribe") {
				char *topic = (char *) words[1].c_str();
				subscribe(sock, words);

				topics.insert(topic);

				continue;
			}

			if (command == "unsubscribe") {
				unsubscribe(sock, words);
			}
		} else {
			int len;
			char buff[1552];

			// primesc lungimea mesajului
			int rc = recv_all(sock, &len, sizeof(len));

			if (rc == 0) {
				break;
			}
			
			// primesc mesajul de lugime len
			recv_all(sock, buff, len);

			// extrag topicul si type ul
			struct udp_msg *message = new udp_msg;
			memset(message, 0, sizeof(struct udp_msg));

			memcpy(message->topic, buff, 50);
			memcpy(&message->type, buff + 50, sizeof(unsigned char));

			// afisez mesajul corespunzator
			write_message(message, buff);
		}
	}

	close(sock);

	return 0;
}
