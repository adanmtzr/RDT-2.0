#include "rdt.h"

static int testOption = 0;

void changeTest(int testNum)
{
	testOption = testNum;
}

int rdt_socket(int address_family, int type, int protocol)
{
	int didUDPSocket = socket(address_family, type, protocol);
	if (didUDPSocket == -1)
	{
		printf("Error starting RDT socket\n");
	}
	else
	{
		printf("Starting RDT socket\n");
	}
	return didUDPSocket;
}

int rdt_bind(int socket_descriptor, const struct sockaddr *local_address, socklen_t address_length)
{
	int didBind = bind(socket_descriptor, local_address, address_length);
	if (didBind == -1)
	{
		printf("Cound not bind RDT socket\n");
	}
	else
	{
		printf("RDT socket binded\n");
	}
	return didBind;
}

/*

***************RDT RECEIVED*********************
*/
int rdt_recv(int socket_descriptor, char *buffer, int buffer_length, int flags, struct sockaddr *from_address, int *address_length)
{
	int totalPackets = ceil((double)buffer_length / 500);
	packet *packets = new packet[totalPackets];

	int packetCountdown = totalPackets;

	while (packetCountdown > 0)
	{
		packet tPacket = udp_rcv(socket_descriptor, buffer, buffer_length, flags, from_address, address_length, tPacket, 0);
		//printf("new pack seq no %d\n", tPacket.seqno);
		if (packets[tPacket.seqno].ackno != 1)
		{
			packetCountdown--;
			packets[tPacket.seqno] = tPacket;
			packets[tPacket.seqno].ackno = 1;

			packet *ackPacket = new packet;
			ackPacket->seqno = tPacket.seqno;
			ackPacket->ackno = packets[tPacket.seqno].ackno;

			sendto(socket_descriptor, ackPacket, 512, flags, from_address, (socklen_t)*address_length);
		}
		//printf("packets left: %d\n", packetCountdown);
	}

	for (int loopNum = 0; loopNum < totalPackets; loopNum++)
	{
		extract_pk(buffer, buffer_length, loopNum, packets[loopNum], loopNum);
	}

	printf("Recieved all packets!");

	return buffer_length;
}

packet udp_rcv(int socket_descriptor, char *buffer, int buffer_length, int flags, struct sockaddr *from_address, int *address_length, packet hPacket, int loopNum)
{
	char tempBuffer[512];
	bzero((void *)tempBuffer, 512);
	int didRecieve = recvfrom(socket_descriptor, &tempBuffer, 512, flags, from_address, (socklen_t *)address_length);
	//printf("RDT Received Length %d\n", didRecieve);
	printf("Recieved a packet!\n");

	if (didRecieve == -1)
	{
		printf("Error recieving packet.\n");
	}

	memcpy(&hPacket, tempBuffer, 512);
	//printf("Packet segment %s\n", hPacket.data);
	return hPacket;
}
int rdt_sendto(int socket_descriptor, char *buffer, int buffer_length, int flags, struct sockaddr *destination_address, int address_length)
{
	int totalPackets = ceil((double)buffer_length / 500);

	printf("Buffer length %d\n", buffer_length);
	printf("Total packets %d\n", totalPackets);
	packet *packets = new packet[totalPackets];

	// Make every packet
	for (int loopNum = 0; loopNum < totalPackets; loopNum++)
	{
		packets[loopNum] = make_pkt(buffer, buffer_length, loopNum, packets[loopNum], loopNum);
	}

	if (testOption == 1)
	{
		printf("Sending packets out of order\n");
	}

	// Send every packet
	for (int loopNum = 0; loopNum < totalPackets; loopNum++)
	{
		if (testOption == 1)
		{
			if (loopNum == 1)
				udt_sendto(socket_descriptor, buffer, buffer_length, flags, destination_address, address_length, &packets[loopNum + 1]);
			else if (loopNum == 2)
				udt_sendto(socket_descriptor, buffer, buffer_length, flags, destination_address, address_length, &packets[loopNum - 1]);
			else
				udt_sendto(socket_descriptor, buffer, buffer_length, flags, destination_address, address_length, &packets[loopNum]);
		}
		if (testOption == 2)
		{
			if (loopNum == 1)
				printf("Packet dropped\n");
			else
				udt_sendto(socket_descriptor, buffer, buffer_length, flags, destination_address, address_length, &packets[loopNum]);
		}
		else
		{
			udt_sendto(socket_descriptor, buffer, buffer_length, flags, destination_address, address_length, &packets[loopNum]);
		}
	}

	int ackCountdown = totalPackets;
	int totalTimeouts = 3;

	while (ackCountdown > 0 && totalTimeouts > 0)
	{
		struct timeval timeout;
		fd_set set;

		timeout.tv_sec = 3;
		timeout.tv_usec = 0;

		FD_ZERO(&set);
		FD_SET(socket_descriptor, &set);

		int resultSet = select(socket_descriptor + 1, &set, NULL, NULL, &timeout);

		if (resultSet == 1)
		{
			char tempBuffer[512];
			bzero((void *)tempBuffer, 512);
			//printf("made it0\n");
			//int didRecieve = recvfrom(socket_descriptor, &tempBuffer, 512, flags, from_address, (socklen_t *)address_length);
			int didRecieveAck = recvfrom(socket_descriptor, &tempBuffer, 512, flags, destination_address, (socklen_t *)&address_length);
			if (didRecieveAck == -1)
			{
				printf("Could not get ack.\n");
				return 0;
			}

			packet *tPacket = new packet;
			memcpy(tPacket, tempBuffer, 512);

			//packet tPacket = udp_rcv(socket_descriptor, buffer, buffer_length, flags, destination_address, address_length, tPacket, 0);
			//printf("new pack seq no %d\n", tPacket->seqno);
			if (packets[tPacket->seqno].ackno != 1)
			{
				ackCountdown--;
				packets[tPacket->seqno].ackno = 0;
			}
			printf("acks left: %d\n", ackCountdown);
		}
		else
		{
			totalTimeouts--;
			for (int loopNum = 0; loopNum < totalPackets; loopNum++)
			{
				if (packets[loopNum].ackno != 1)
				{
					sendto(socket_descriptor, &packets[loopNum], 512, flags, destination_address, address_length);
				}
			}
			//timeout.tv_sec = 3;
		}
	}

	printf("Recieved all acknowledgements!\n");

	return buffer_length;
}

void udt_sendto(int socket_descriptor, char *buffer, int buffer_length, int flags, struct sockaddr *destination_address, int address_length, packet *hPacket)
{
	/*  //simulate packet Corruption
  time_t t;
  int randNum;
  // initize the random number
  srand((unsigned) time(&t));

  // random number between 0 and 1999
  randNum = rand()%2000;
  printf("Random number generated %d\n", randNum);
  */

	//printf("Packet segment %s", hPacket.data);
	//printf("Packet segment %s\n", hPacket->data);

	int didSend = sendto(socket_descriptor, hPacket, 512, flags, destination_address, address_length);
	if (didSend == -1)
	{
		printf("Error sending packet");
	}
}

int rdt_close(int fildes)
{
	int didClose = close(fildes);
	if (didClose == -1)
	{
		printf("Could not close RDT socket\n");
	}
	else
	{
		printf("Closing RDT socket\n");
	}
	return didClose;
}

packet make_pkt(char *buffer, int length, uint32_t seqNo, packet hPacket, int loopNum)
{

	// Attach data to packet
	for (int dataLoop = 0; dataLoop < 500; dataLoop++)
	{
		hPacket.data[dataLoop] = '\0';
		int dataPoint = (loopNum * 500) + dataLoop;
		(dataPoint < length) ? hPacket.data[dataLoop] = buffer[dataPoint] : hPacket.data[dataLoop] = '\0';
	}
	hPacket.seqno = seqNo;
	hPacket.cksum = getCheckSum(hPacket);
	hPacket.ackno = 0;

	return hPacket;
}
packet extract_pk(char *buffer, int length, uint32_t seqNo, packet hPacket, int loopNum)
{
	//extract packets

	for (int dataLoop = 0; dataLoop < 500; dataLoop++)
	{
		int dataPoint = loopNum * 500 + dataLoop;
		if (dataPoint < length)
		{
			buffer[dataPoint] = hPacket.data[dataLoop];
		}
	}

	unsigned short calcCheckSum = getCheckSum(hPacket);
	if (calcCheckSum == hPacket.cksum)
	{

		printf("Good packet\n");
	}
	else
	{
		printf("Error: Loss packet\n");
	}

	return hPacket;
}

unsigned short checkSum(unsigned char *addr, int nBytes)
{
	unsigned int sum = 0;
	//Summing loop
	while (nBytes > 1)
	{
		sum = sum + *(unsigned short *)addr;
		addr++;
		nBytes -= 2;
	}
	//Add the left-Over byte, if any
	if (nBytes > 0)
	{
		sum = sum + *((unsigned char *)addr);
	}
	//Fold 32-bit to 16-bits
	while (sum >> 16)
		sum = (sum & 0XFFFF) + (sum >> 16);
	//  printf("\tYour CheckSum  ++++> %04X \n",((unsigned short)~sum));
	return ((unsigned short)~sum);
}

//calculates the Checksum of the given packet
unsigned short getCheckSum(packet cpacket)
{

	unsigned short pakCksum;
	cpacket.len = strlen(cpacket.data);

	cpacket.len = 500;
	pakCksum = checkSum((unsigned char *)cpacket.data, cpacket.len);
	return pakCksum;
}
