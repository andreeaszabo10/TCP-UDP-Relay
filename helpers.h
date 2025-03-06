
/**
 * Error checking macro.
 * Example:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */
#define DIE(assertion, call_description)	                    \
	do {									                    \
		if (assertion) {					                    \
			fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);	\
			perror(call_description);		                    \
			exit(EXIT_FAILURE);				                    \
		}									                    \
	} while(0)

// structura prin care trimit mesaje cu comenzi catre server
struct command_message {
	char topic[51];
	int type;
};

// structura pentru a trimite eficient mesaje peste tcp
struct tcp_msg {
	char *data;
	int len;
};

// structura pentru a separa mesajele venite de la clientii udp
struct udp_msg {
    char topic[51];
	char data[1500];
	unsigned char type;
};

