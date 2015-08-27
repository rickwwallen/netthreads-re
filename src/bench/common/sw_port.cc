
#include "sw_port.h"


/*********************************************************************************/
int RECIRC = 0;

PcapPort::PcapPort(int port_num_, char *pcapfile) 
{
  pcap = NULL;
  can_open = true;
  port_num = port_num_;

  char * pch = strtok(pcapfile,",");
  while (pch != NULL)
    {
      next_file_list.push_back(pch);
      pch = strtok (NULL, ",");
    }
  printf("port %d found %d trace(s). recirc %d\n", port_num, next_file_list.size(), RECIRC);

  next_file();

  char fname[16];
  sprintf(fname, "pcap%d.trace", port_num);
  printf("Output for port %d will be in %s\n", port_num, fname);

  unlink(fname);

  fdout = open(fname, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
  if(fdout == -1)
    {
      perror("could not open pcap file for writing");
      assert(fdout != -1);
    }

  pcap_file_header fhd;
  fhd.magic = 0xa1b2c3d4;
  fhd.version_major = 2;
  fhd.version_minor = 4;
  fhd.thiszone = 0;
  fhd.sigfigs = 0;
  fhd.snaplen = 65535;
  fhd.linktype = 1; 
  write(fdout, &fhd, sizeof(pcap_file_header));
}


bool PcapPort::next_file() 
{
 if (pcap) {
    pcap_close(pcap);
    pcap = NULL;
  }

 if((!RECIRC) && (next_file_list.size() == 0))
   {
     return false;
   }
 else if(next_file_list.size() == 0)
   {
     next_file_list = old_file_list;
     old_file_list.clear();
     assert(next_file_list.size() != 0);
   }

 const char* pcapfile = next_file_list.front().c_str();
 old_file_list.push_back(next_file_list.front());

 char errbuf[PCAP_ERRBUF_SIZE]; 
 pcap = pcap_open_offline(pcapfile, errbuf);
 bool ret = true;

 if(!pcap) 
   { 
     printf("error opening pcap file \"%s\": %s \n", pcapfile, errbuf);
     ret = false;
   }
 else
   printf("opened trace %s\n", pcapfile);

 next_file_list.pop_front();
 current_done = false;
 return ret;
}

PcapPort::~PcapPort() {
  if (pcap) {
    pcap_close(pcap);
    pcap = NULL;
  }
  close(fdout);
}



int PcapPort::fetch_pkt(char *buff, bool& new_location) {
  pcap_pkthdr *hdr;
  const u_char *data;
  int result;
  new_location = false;

 tryonce:
  if (pcap) {

    result = pcap_next_ex(pcap, &hdr, &data);

    if (current_done || ((result < 1) && can_open))
      {
	printf("port %d finished reading packet trace, current_done %d, can_open %d\n", 
	       port_num, current_done, can_open);
	new_location = next_file();
	goto tryonce;
	
      } 
    else if (result >= 1) {
      struct ioq_header *ioq = (struct ioq_header *) buff;
      
      int size = hdr->caplen;
      // pad the packet to 60 bytes
      if (size < 60) {
        memset(buff + sizeof(struct ioq_header) + size, 0, 60 - size);
        size = 60;
      }

      // fill ioq header
      ioq->dst_port = 0;
      ioq->word_length = htons(size / 8 + (size % 8 > 0));
      ioq->src_port = htons(port_num);
      ioq->byte_length = htons(size);

      // copy actual packet
      memcpy(buff + sizeof(struct ioq_header), data, hdr->caplen);
      
      return size + sizeof(ioq_header);
    }
  }

  
  return 0;
}

void PcapPort::send_pkt(char *buff, int size) {
  pcap_pkthdr head;
  head.ts.tv_usec = 0;
  head.ts.tv_sec  = 0;
  head.caplen = head.len = size;
  write(fdout, &head, sizeof(pcap_pkthdr));

  write(fdout, buff, size);
}


/*-----------------------------------------------*/
#if 0
// this is a tcp syn packet
char mypack[] = {0,0,0,0,0,0,0,0,
                 0x00,0xff,0x32,0x99,0x10,0x00,0x00,0xff,0x83,0x82,0x46,0x79,0x08,0x00,0x45,0x00,
		 0x00,0x3c,0xeb,0xb1,0x40,0x00,0x40,0x06,0x3b,0x06,0x0a,0x01,0x00,0x01,0x0a,0x01,
		 0x00,0x02,0x8d,0x1f,0x27,0x10,0x98,0x57,0x00,0x43,0x00,0x00,0x00,0x00,0xa0,0x02,
		 0x16,0xd0,0x9b,0xbd,0x00,0x00,0x02,0x04,0x05,0xb4,0x04,0x02,0x08,0x0a,0x15,0x2c,
		 0xf7,0xa4,0x00,0x00,0x00,0x00,0x01,0x03,0x03,0x02};

void pp()
{
  char* mem_addr = mypack;
#define len 82
  for(int i=0; i<len; )
    {
      for(int j=0; (j<8)&&(i<len);j++)
        {
          for(int k=0; (k<2)&&(i<len);(k++,i++))
	    {
 	      int t;
	      printf("%02x", (((int)(*mem_addr)) & 0xff));
	      mem_addr++;
	    }
          printf(" ");
        }
      printf("\n");
    }
}

int main(void) {
  printf("netport test\n");

  IpChangePort tp(0,0,NULL);

  /*TapPort tp(0);
  tp.init("tap0");*/

  char buff[2000];
  bool new_loc;

  //while (1) 
    {
    //int r = tp.fetch_pkt(buff);

      pp();
      int r = tp.fetch_pkt(mypack, new_loc);
    pp();

    //    if (r != 0) {
      printf("Fetched packet. Result is %d\n", r);
      //    }
  }


  return 0;
}
#endif
