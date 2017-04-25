#include <iostream>
#include <fstream>
#include <string>
#include "rdt.h"

using namespace std;

int main(int argc, char *argv[]) {

    struct sockaddr_in saddr, caddr;

    char buffer[2000];

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(atoi(argv[1]));

    // Handle on RDT server
    int rdtHandle = rdt_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    rdt_bind(rdtHandle, (struct sockaddr*) &saddr, sizeof (caddr));
    
    // TODO: Send and recieve with client
    
    //for (;;){
	int clientAddressSizeHandle = sizeof(caddr);
	int rdtDataReceived = rdt_recv(rdtHandle, buffer, sizeof(buffer), 0, (struct sockaddr *)&caddr, &clientAddressSizeHandle);
	printf("\nPayload: %s\n", buffer);
	printf("\nPayload buffer length : %lu\n", strlen(buffer));
    //}
    rdt_close(rdtHandle);
    return 0;
}
