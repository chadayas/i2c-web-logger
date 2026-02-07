#include<libs.h>
#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>

void debug(int res){
	if(res < 0){
		std::cout << "Problem with connection"<<std::endl;
		std::cout << 
	}
}


extern "C" void app_main(void){
	
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	int bd = bind()

}
