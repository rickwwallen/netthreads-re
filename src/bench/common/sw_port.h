#ifndef TAP_H
#define TAP_H


#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <assert.h>
#define PCAP_DONT_INCLUDE_PCAP_BPF_H
#include "pcap.h"
/** #include "pktbuff.h" **/

#include <string>
#include <list>
using namespace std;
/** From pktbuff **/
struct ioq_header {
  unsigned short dst_port;
  unsigned short word_length;
  unsigned short src_port;
  unsigned short byte_length;
};
/** End From pktbuff **/
/*
  Read arriving packets from a pcap file. The timing of packets is ignored.
  Every call to fetch_pkt will return a packet until there are none left in
  the file.
 */
class PcapPort {
 public:
  PcapPort(int port_num, char *pcapfile);
  ~PcapPort();
  int fetch_pkt(char *buff, bool& new_location);
  void send_pkt(char *buff, int size);

 private:
  bool current_done;
  bool next_file();
  bool can_open_next;
  list<string> next_file_list;
  list<string> old_file_list;
  pcap_t *pcap;
  int fdout;
  bool can_open;
  int port_num;
};

class PktPort {
 public:
  PktPort(const char* rx_dev, const char* tx_dev);
  ~PktPort();
  char* fetch_pkt();
  void send_pkt(char* start_addr, char * end_addr);

 private:
  int rx_sock;
  int tx_sock;
  int num_rx, num_tx;
};



#endif
