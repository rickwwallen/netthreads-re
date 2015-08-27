#include "sw_port.h"
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>

#define IOQ_PORT 0
unsigned char packet_buffer[2048];

int CreateRawSocket(int protocol_to_sniff)
{
  int rawsock = socket(PF_PACKET, SOCK_RAW, htons(protocol_to_sniff));
  if(rawsock == -1)
    {
      perror("Error creating raw socket: ");
      exit(-1);
    }
  return rawsock;
}

int BindRawSocketToInterface(const char *device, int rawsock, int protocol)
{
  struct sockaddr_ll sll;
  struct ifreq ifr;

  bzero(&sll, sizeof(sll));
  bzero(&ifr, sizeof(ifr));
	
  /* First Get the Interface Index  */
  strncpy((char *)ifr.ifr_name, device, IFNAMSIZ);
  if((ioctl(rawsock, SIOCGIFINDEX, &ifr)) == -1)
    {
      printf("Error getting Interface index !\n");
      exit(-1);
    }

  /* Bind our raw socket to this interface */
  sll.sll_family = AF_PACKET;
  sll.sll_ifindex = ifr.ifr_ifindex;
  sll.sll_protocol = htons(protocol); 

  if((bind(rawsock, (struct sockaddr *)&sll, sizeof(sll)))== -1)
    {
      perror("Error binding raw socket to interface\n");
      exit(-1);
    }

  return 1;
	
}

void PrintPacketInHex(unsigned char *packet, int len)
{
#if 1
  unsigned char *p = packet;
	int pktlen = len +8;
  printf("\n\n---------Packet---Starts----\n\n");
  while((pktlen)--)
    {
      printf("%.2x ", *p);
      p++;
    }
  printf("\n\n--------Packet---Ends-----\n\n");

  //sleep(1);
#endif
}

/*----------------------------------------*/

PktPort::PktPort(const char* rx_dev, const char* tx_dev)
{
  rx_sock = CreateRawSocket(ETH_P_ALL);
  BindRawSocketToInterface(rx_dev, rx_sock, ETH_P_ALL);

  if(strcmp(rx_dev,tx_dev) != 0)
    {
      tx_sock = CreateRawSocket(ETH_P_ALL);
      BindRawSocketToInterface(tx_dev, tx_sock, ETH_P_ALL);
    }
  else // use the same socket
    {
      tx_sock = rx_sock;
    }
  num_rx = num_tx = 0;
}

PktPort::~PktPort()
{
  close(rx_sock);
  if(tx_sock != rx_sock)
    close(tx_sock);
}

char* PktPort::fetch_pkt()
{
  int len;
  struct sockaddr_ll packet_info;
  unsigned int packet_info_size = sizeof(packet_info);
	
  len = recvfrom(rx_sock, 
		 packet_buffer + sizeof(struct ioq_header), 
		 2048, 
		 0,
		 (struct sockaddr*)&packet_info,
		 &packet_info_size);
  if(len == -1)
    {
      perror("Recv from returned -1: ");
      exit(-1);
    }
  else
    {
      printf("Packet %d has been received successfully !!\n", num_rx++);

      PrintPacketInHex(packet_buffer, len);
      if (len <60){len = 60;}
      struct ioq_header *ioq = (struct ioq_header *) packet_buffer;
      // fill ioq header
      ioq->dst_port = 0;
      ioq->word_length = htons(len / 8 + (len % 8 > 0));
      ioq->src_port = htons(IOQ_PORT);
      ioq->byte_length = htons(len);
    }

  return (char*)packet_buffer;
}

void PktPort::send_pkt(char* start_addr, char * end_addr)
{
  printf(" Inside writeToTap");
  int pkt_len = end_addr - start_addr - 7;
  unsigned char* pkt = (unsigned char*)(start_addr+8);


  int sent = write(tx_sock, pkt, pkt_len);

  if(sent != pkt_len)
    perror("Error sending packet");
  else
    printf("Packet %d sent successfully\n", num_tx++);
}

