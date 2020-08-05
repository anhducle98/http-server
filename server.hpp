#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <cstring>
#include <thread>

#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <signal.h>

#include "request_handler.hpp"

class Server {
	static const int NUM_EPOLL_FD = 10000;
	static const int EPOLL_TIMEOUT = 1000;
	int port;
	int backlog;
	int num_workers;

	std::string root;
	std::vector<std::thread> workers;
	std::vector<int> epoll_fds;

	static void kill_connection(int epfd, RequestHandler *handler) {
		// fprintf(stderr, "kill_connection epfd=%d, fd=%d\n", epfd, handler->fd);
		epoll_ctl(epfd, EPOLL_CTL_DEL, handler->fd, NULL);
		shutdown(handler->fd, SHUT_RDWR);
		close(handler->fd);
		delete handler;
	}

	static void work(int epfd) {
		epoll_event events[NUM_EPOLL_FD];

		for (;;) {
			int num_ready = epoll_wait(epfd, events, NUM_EPOLL_FD, 1000);
			if (num_ready <= 0) {
				if (num_ready < 0) perror("epoll_wait");
				continue;
			}
			// fprintf(stderr, "worker handle epfd=%d, num_ready=%d\n", epfd, num_ready);
			for (int i = 0; i < num_ready; ++i) {
				RequestHandler *handler = (RequestHandler*) events[i].data.ptr;
				if (events[i].events & EPOLLIN) {
					int status = handler->handle_request();
					if (status == 0) {
						events[i].events = EPOLLOUT;
						epoll_ctl(epfd, EPOLL_CTL_MOD, handler->fd, events + i);
					} else if (status < 0) {
						kill_connection(epfd, handler);
					}
				} else if (events[i].events & EPOLLOUT) {
					int status = handler->handle_response();
                    // fprintf(stderr, "handle_response status=%d\n", status);
					if (status < 0 || !handler->keep_alive()) {
						kill_connection(epfd, handler);
					} else {
						events[i].events = EPOLLIN;
						epoll_ctl(epfd, EPOLL_CTL_MOD, handler->fd, events + i);
					}
				} else {
					kill_connection(epfd, handler);
				}
			}
		}
	}

	int setup_sockfd() {
		int status;
		addrinfo hints;
		addrinfo *server_info;
		
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		char port_str[7];
		sprintf(port_str, "%d", port);

		if ((status = getaddrinfo(NULL, port_str, &hints, &server_info)) != 0) {
			fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
			return -1;
		}

		int sockfd;
		bool success = false;
		for (auto p = server_info; p; p = p->ai_next) {
			sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
			if (sockfd < 0) {
				perror("socket");
				continue;
			}

			int yes = 1;
			if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
				perror("setsockopt");
				return -1;
			}

			if (setsockopt(sockfd, IPPROTO_TCP, TCP_QUICKACK, &yes, sizeof(int)) < 0) {
				perror("setsockopt");
				return -1;
			}

			if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
				perror("bind");
				close(sockfd);
				continue;
			}
			success = true;
			break;
		}

		freeaddrinfo(server_info);
		if (!success) {
			return -1;
		}

		return sockfd;
	}

	static void* get_in_addr(sockaddr *sa) {
		if (sa->sa_family == AF_INET) {
			return &(((sockaddr_in*)sa)->sin_addr);
		}
		return &(((sockaddr_in6*)sa)->sin6_addr);
	}

	static void print_client_addr(const sockaddr_storage &client_addr) {
		static char buf[INET6_ADDRSTRLEN];
		inet_ntop(client_addr.ss_family, get_in_addr((sockaddr*)&client_addr), buf, sizeof buf);
		printf("new connection from %s\n", buf);
	}

	static void make_non_block(int fd) {
		int flags = fcntl(fd, F_GETFL);
		if (flags < 0) {
			perror("fcntl");
			return;
		}
		if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
			perror("fcntl");
		}
	}

public:
	int run() {
        printf("root dir = \"%s\"\n", root.c_str());
        printf("port = %d\n", port);
		printf("backlog = %d\n", backlog);
		printf("num_workers = %d\n", num_workers);

		int sockfd = setup_sockfd();
		if (sockfd < 0) return -1;

		signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE

		if (listen(sockfd, backlog) < 0) {
			perror("listen");
			return -1;
		}

		for (int i = 0; i < num_workers; ++i) {
			int epfd = epoll_create(NUM_EPOLL_FD);
			if (epfd < 0) {
				perror("epoll_create");
				return -1;
			}
			epoll_fds.push_back(epfd);
			workers.push_back(std::thread(work, epfd));
		}
        
		printf("Server started, listening on port %d\n", port);
		
		size_t worker_id = 0;
		for (;;) {
			sockaddr_storage client_addr;
			socklen_t addr_size = sizeof client_addr;
			int cur_fd = accept(sockfd, (sockaddr*) &client_addr, &addr_size);
			if (cur_fd <= 0) {
				perror("accept");
				continue;
			}

			print_client_addr(client_addr);

			make_non_block(cur_fd);
			epoll_event ev;
			ev.events = EPOLLIN;
			ev.data.ptr = new RequestHandler(root, cur_fd);
			epoll_ctl(epoll_fds[worker_id], EPOLL_CTL_ADD, cur_fd, &ev);
			if ((++worker_id) == workers.size()) worker_id = 0;
		}

		return 0;
	}

    Server(const std::string &root, int port, int num_workers, int backlog = SOMAXCONN) {
		this->port = port;
		this->backlog = backlog;
		this->root = root;
		this->num_workers = num_workers;
	}
};

#endif // SERVER_HPP
