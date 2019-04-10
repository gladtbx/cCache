#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <unordered_map>
#include <iostream>
#include <chrono>
//#include <fcntl.h>


#define PORT 9407
#define BUFSIZE (32768)

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
    long int setuptime = 0;
    long int checktime = 0;
    long int inserttime = 0;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //fcntl(server_fd, F_SETFL, O_NONBLOCK);
    // Forcefully attaching socket to the port 9407
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(PORT);

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
	    setuptime = 0;
	    checktime = 0;
	    inserttime = 0;
	    //std::cout  << address.sin_addr << ":" << address.sin_port <<std::endl;
		while (1) {
			valread = read( new_socket , buffer, BUFSIZE);
//			printf("Read %d of bytes\n",valread);
			if(valread<0){
				std::cout<<"Read Error"<< std::endl;
				close(new_socket);
				break;
			}
			std::chrono::high_resolution_clock::time_point s = std::chrono::high_resolution_clock::now();
			buffer[valread] = NULL;
//			printf("MSG Received: %s\n",buffer);
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
			//printf("%s\n",body.c_str());
	      	std::chrono::high_resolution_clock::time_point e = std::chrono::high_resolution_clock::now();
	      	setuptime += (std::chrono::duration_cast<std::chrono::nanoseconds> (e-s)).count();
			if(header == "check"){
				auto duration = s.time_since_epoch();
				//printf("Current Time for check: %ld, %ld\n",std::chrono::duration_cast<std::chrono::nanoseconds>(r.time_since_epoch()).count(),std::chrono::duration_cast<std::chrono::nanoseconds> (duration).count());
				//printf("Check if cache hit\n");
				s = std::chrono::high_resolution_clock::now();
				std::unordered_map<std::string, std::string>::iterator get = cache.find(body);
				e = std::chrono::high_resolution_clock::now();
				checktime += (std::chrono::duration_cast<std::chrono::nanoseconds> (e-s)).count();
				s = std::chrono::high_resolution_clock::now();
				if(get != cache.end()){
					//Cache Hit!
					cachehit++;
				    send(new_socket , get->second.c_str() , get->second.size() , 0 );
				}else{
					cachemiss++;
					send(new_socket, "N ", 2, 0);
				}
				e = std::chrono::high_resolution_clock::now();
				duration = e.time_since_epoch();
				//printf("Current Time for reply: %ld,%d,%c\n",std::chrono::duration_cast<std::chrono::nanoseconds> (duration).count(),valread,rep);
				setuptime += (std::chrono::duration_cast<std::chrono::nanoseconds> (e-s)).count();
				continue;
			}
			if(header == "insert"){
				send(new_socket, "Ack", 3, 0);
				s = std::chrono::high_resolution_clock::now();
				std::size_t ans_pos = body.find_first_of(' ');
				if(ans_pos == std::string::npos){
					printf("Error, msg does not contain result for insertion\n");
					//close(new_socket);
					break;
				}

				std::string qlen = body.substr(0,ans_pos);
				int qlength = std::stoi(qlen);
				std::string result(buffer+8+ans_pos,valread-qlength-ans_pos-8);
				body = std::string(buffer+(valread-qlength),qlength);

//				std::cout<< body << " : " << result<< std::endl;
				e = std::chrono::high_resolution_clock::now();
				setuptime += (std::chrono::duration_cast<std::chrono::nanoseconds> (e-s)).count();
				s = std::chrono::high_resolution_clock::now();
				cache[body] = result;
				e = std::chrono::high_resolution_clock::now();
				inserttime += (std::chrono::duration_cast<std::chrono::nanoseconds> (e-s)).count();
				continue;
			}
		}
		printf("Setup time: %ld\nCheck time: %ld\nInsert time: %ld\n",setuptime,checktime,inserttime);
	}
    return 0;
}
