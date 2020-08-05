#include "server.hpp"

void usage() {
    fprintf(stderr, "usage: http-server [-p <port>] [-f <root dir>] [-n <num workers>]\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    const char *root = "www";
    int port = 8080;
    int num_workers = 2;

    int opt;
    while ((opt = getopt(argc, argv, "p:f:n:h")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'f':
                root = strdup(optarg);
                break;
            case 'n':
                num_workers = atoi(optarg);
                break;
            default:
                usage();
        }
    }

	Server server(root, port, num_workers);
	return server.run();
}
