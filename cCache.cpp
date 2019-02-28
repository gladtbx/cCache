#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <unordered_map>
#include <iostream>

#define PORT 9407
#define BUFSIZE (4096)

int main(int argc, char const *argv[])
{
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFSIZE] = {0};
    std::unordered_map<std::string,std::string> cache;
    unsigned long cachehit = 0;
    unsigned long cachemiss = 0;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 9407
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = PORT;

    // Forcefully attaching socket to the port 9407
    if (bind(server_fd, (struct sockaddr *)&address,
                                 sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

	bool isRunning = true;

	while (isRunning) {
		printf("Waiting for a client to connect...");
	    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
	                       (socklen_t*)&addrlen))<0)
	    {
	        perror("accept");
	        exit(EXIT_FAILURE);
	    }
	    //std::cout  << address.sin_addr << ":" << address.sin_port <<std::endl;
		while (1) {
			valread = read( new_socket , buffer, BUFSIZE);
			if(valread<0){
				std::cout<<"Read Error"<< std::endl;
				close(new_socket);
				break;
			}
			buffer[valread] = NULL;
			//printf("MSG Received: %s\n",buffer);
			std::string msg(buffer);
			if(msg == "CLOSE"){
				printf("Good talking to you, closing socket now\n");
				printf("Cache Hit Until Now: %lu\nCache Miss Until Now: %lu\n",cachehit,cachemiss);
				close(new_socket);
				break;
			}
			std::size_t header_pos = msg.find_first_of(' ');
			if(header_pos == std::string::npos){
				printf("Error, msg cannot be decoded!\n");
				close(new_socket);
				break;
			}
			std::string header = msg.substr(0,header_pos);
			std::string body = msg.substr(header_pos+1);
			if(header == "check"){
				//printf("Check if cache hit\n");
				std::unordered_map<std::string, std::string>::iterator get = cache.find(body);
				if(get != cache.end()){
					//Cache Hit!
					cachehit++;
					std::string reply = "G " + get->second;
				    send(new_socket , reply.c_str() , reply.size() , 0 );
				}else{
					cachemiss++;
					send(new_socket, "N ", 2, 0);
				}
				continue;
			}
			if(header == "insert"){
				std::size_t ans_pos = body.find_first_of(' ');
				if(ans_pos == std::string::npos){
					printf("Error, msg does not contain result for insertion\n");
					close(new_socket);
					break;
				}
				std::string result = body.substr(0,ans_pos);
				body = body.substr(ans_pos+1);
				cache[body] = result;
				continue;
			}
		}
	}
    return 0;
}
