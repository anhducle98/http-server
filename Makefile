CXX = g++
CXXFLAGS = -std=c++11 -O3 -W # -Werror -Wall -pedantic
PROG = http-server
SRCS = main.cpp tokenizer.hpp http_request.hpp http_response.hpp request_handler.hpp server.hpp
all: $(PROG)

$(PROG): $(SRCS)
	        $(CXX) $(CXXFLAGS) main.cpp -o $(PROG) -lpthread

.PHONY:clean

clean:
	        rm -rf $(PROG)
