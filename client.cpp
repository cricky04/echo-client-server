#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include <thread>
#include <arpa/inet.h>


using namespace std;

void usage() {
	cout << "syntax : echo-client <ip> <port>\n";
	cout << "sample : echo-client 192.168.10.2 1234\n";
}

struct Param {
	struct in_addr ip{0};
	uint16_t port{0};

	bool parse(int argc, char* argv[]) {
		for (int i = 1; i < argc; i++) {
			int res = inet_pton(AF_INET, argv[i++], &ip);
			switch (res) {
				case 1: break;
				case 0: cerr << stderr, "not a valid network address\n"; return false;
				case -1: perror("inet_pton"); return false;
			}
			port = atoi(argv[i++]);
		}
		return (ip.s_addr != 0) && (port != 0);
	}
} param;

void recvThread(int sd) {
	cout << "connected\n";
	static const int BUFSIZE = 65536;
	char buf[BUFSIZE];
	while (true) {
		ssize_t res = recv(sd, buf, BUFSIZE - 1, 0);
		if (res == 0 || res == -1) {
			cerr << "recv return "<< res << endl;
			perror("recv");
			break;
		}
		buf[res] = '\0';
		cout << buf <<endl;
		fflush(stdout);
	}
	cout << "disconnected\n";
	close(sd);
	exit(0);
}

int main(int argc, char* argv[]) {
	if (!param.parse(argc, argv)) {
		usage();
		return -1;
	}

	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket");
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(param.port);
	addr.sin_addr = param.ip;
	memset(&addr.sin_zero, 0, sizeof(addr.sin_zero));

	int res = connect(sd, (struct sockaddr *)&addr, sizeof(addr));
	if (res == -1) {
		perror("connect");
		return -1;
	}

	thread t(recvThread, sd);
	t.detach();

	while (true) {
		static const int BUFSIZE = 65536;
		char buf[BUFSIZE];
		scanf("%s", buf);
		strcat(buf, "\r\n");
		ssize_t res = send(sd, buf, strlen(buf), 0);
		if (res == 0 || res == -1) {
			cerr << "send return " << res << endl;
			perror("send");
			break;
		}
	}
	close(sd);
}