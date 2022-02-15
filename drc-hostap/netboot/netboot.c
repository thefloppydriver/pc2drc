/* http://brokestream.com/netboot.html

  To build it: gcc -o netboot netboot.c

  netboot.c
  Version 2010-01-19

  Copyright (C) 2007-2010 Ivan Tikhonov

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Ivan Tikhonov, kefeer@brokestream.com

*/


#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include <pwd.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <stdint.h>

void tftpd(int s) {
	static int f;
	static int blksize = 512;
	static int last_sent = 0;
	static int was_last = 0;
	{
		unsigned char p[1500];
		struct sockaddr_in peer;
		int peerl = sizeof(peer);
		int n = recvfrom(s,p,1500,0,(struct sockaddr *)&peer,&peerl);
		if(p[1] == 1) {
			unsigned char o[1500];
			unsigned char *pp = p+2;
			unsigned char *epp = p+n;
			while(pp<epp&&*pp) pp++; pp++;
			while(pp<epp&&*pp) pp++; pp++;

			printf("rrq %s\n", p+2);

			f = open(p+2,O_RDONLY);
			if(f == -1) {
				o[0] = 0; o[1] = 5;
				o[2] = 0; o[3] = 1;
				strcpy(o+4,"Not found");
				o[13] = 0;
				sendto(s,o,14,0,(struct sockaddr *)&peer,peerl);
				printf("error\n");
				return;
			}

			o[0] = 0;
			if(pp<epp&&*pp) {
				unsigned char *p = o+2;
				struct stat st;
				fstat(f,&st);
				o[1] = 6;
				while(pp<epp) {
					if(strcmp(pp,"tsize") == 0) {
						strcpy(p,"tsize"); p+=6;
						p+=sprintf(p,"%u", (int)st.st_size); *p++ = 0;
						pp+=6; pp+=strlen(pp)+1;
					} else if(strcmp(pp,"blksize") == 0) {
						strcpy(p,"blksize"); p+=8; pp+=8;
						blksize = atoi(pp);
						strcpy(p,pp); p+=strlen(p)+1; pp+=strlen(pp)+1;
					} else {
						pp+=strlen(pp)+1; pp+=strlen(pp)+1;
					}
				}
				sendto(s,o,p-o,0,(struct sockaddr *)&peer,peerl);
			}

		} else if(p[1] == 4) {
			int no = (p[2]<<8)|p[3];
			if(was_last) { printf("end\n"); was_last = 0; blksize = 512; last_sent = 0; close(f); f = -1; }
			if(no++ == last_sent) {
				int n;
				unsigned char o[10240] = {0,3,(no>>8)&0xff,no&0xff};
				lseek(f,(no-1)*blksize,SEEK_SET);
				n = read(f,o+4,blksize);
				sendto(s,o,n+4,0,(struct sockaddr *)&peer,peerl);
				last_sent = no;
				if(n<blksize) was_last = 1;
			}
		}
	}
}

int main(int argc, char *argv[]) {
	int s = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	int r = -1;
	int t = -1;

	unsigned char macpat[6]; int macskip = 6;

	in_addr_t bc = 0;
	in_addr_t sc = 0;
	in_addr_t cc = 0;

	if(argc > 1 && argc < 5) {
		fprintf(stderr, "Usage  : ./netboot <broadcast ip> <interface ip> <ip to assign> <mac>\n");
		fprintf(stderr, "Example: ./netboot 192.168.0.255 192.168.0.1 192.168.0.55 00:11:ee:ff:66:ef\n");
		fprintf(stderr, "Example: ./netboot 192.168.0.255 192.168.0.1 192.168.0.55 -66-ef\n");
		fprintf(stderr, "\nTo find out who requesting boot run: ./netboot\n");
		exit(1);
	}

	if(argc == 5) {
 		bc = inet_addr(argv[1]);
 		sc = inet_addr(argv[2]);
		cc = inet_addr(argv[3]);

		{ char *p = argv[4]; while(*p) {
			int d;
			if(p[0]>='0'&&p[0]<='9') { d = (p[0]-'0')<<4; }
			else if(p[0]>='a'&&p[0]<='f') { d = (p[0]-'a'+0xa)<<4; }
			else if(p[0]>='A'&&p[0]<='F') { d = (p[0]-'A'+0xa)<<4; }
			else { p++; continue; }
			if(p[1]>='0'&&p[1]<='9') { d = d|(p[1]-'0'); }
			else if(p[1]>='a'&&p[1]<='f') { d = d|(p[1]-'a'+0xa); }
			else if(p[1]>='A'&&p[1]<='F') { d = d|(p[1]-'A'+0xa); }

			macpat[6-macskip] = d;
			macskip--;
			p+=2;
		}}
	}

	if(cc) {
		fprintf(stderr, "mac pattern:");
		{ int i = macskip;
		  while(i--) { fprintf(stderr,"?? "); }
		  i = 0; while(i<(6-macskip)) { fprintf(stderr, "%02x ",macpat[i]); i++; }
		}
		fprintf(stderr, "\n");
	}

	{ struct sockaddr_in b = { AF_INET, htons(67), {0xffffffff} };
	  if(bind(s, (struct sockaddr *)&b, sizeof(b)) != 0) {
		fprintf(stderr, "Can not bind broadcast address. DHCP will not work! Try run it as root?\n");
	  }
	}

	if(cc)
	{ struct sockaddr_in b = { AF_INET, htons(67), {sc} };
	  r = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	  if(bind(r, (struct sockaddr *)&b, sizeof(b)) != 0) {
		fprintf(stderr, "Can not bind server address. DHCP WILL NOT WORK! Try run it as root?\n");
	  }
	}

	if(cc)
	{ struct sockaddr_in b = { AF_INET, htons(69), {sc} };
 	  t = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	  if(bind(t, (struct sockaddr *)&b, sizeof(b)) != 0) {
		fprintf(stderr, "Can not bind tftp server address. TFTP WILL NOT WORK! Try run it as root?\n");
	  }
	}

	{ int v = 1;
	  setsockopt(r,SOL_SOCKET,SO_BROADCAST,&v,sizeof(v));
	}

	{
		struct passwd *nobody;
		nobody = getpwnam("nobody");
		if (nobody && nobody->pw_uid) setuid(nobody->pw_uid);
	}


	for(;;) {
		fd_set rset;
		int n;

		FD_ZERO(&rset);
		FD_SET(s,&rset);
		if(t != -1) FD_SET(t,&rset);
		n = select((t>s?t:s)+1,&rset,0,0,0);

		if(cc&&FD_ISSET(t,&rset)) {
			tftpd(t);
		}

		if(FD_ISSET(s,&rset)) {
			int type, pos97 = -1, pos12=-1;
			unsigned char p[1500] = {0xff};
			recv(s,p,1500,0);
			if(p[0] == 2) continue;


			{ int i = 240;
			  for(;i<1500;) {
				if(p[i] == 0xff) break;
				if(p[i] == 0x0) { i++; continue; }
				if(p[i] == 53) { type = p[i+2]; }
				if(p[i] == 97) { pos97 = i+1; }
				if(p[i] == 12) { pos12 = i+1; }
				i += p[i+1] + 2;
			  }
			}

			printf("request %u from %02x-%02x-%02x-%02x-%02x-%02x ",type,p[28],p[29],p[30],p[31],p[32],p[33]);
			if(pos12!=-1) { printf("(%.*s)\n", p[pos12],p+pos12+1);
			} else { printf("(%s)\n", p+44); }
			if(!cc) continue;

			if(memcmp(p+28+macskip,macpat,(6-macskip))) continue;

			printf("matched\n");

			if(type == 1 || type == 3) {
				int i,m;
				p[0] = 2;
				p[12] = 0; p[13] = 0; p[14] = 0; p[15] = 0;

				*(uint32_t*)(p+16)=(uint32_t)cc;
				*(uint32_t*)(p+20)=(uint32_t)sc;

				i = 240;


				if(pos97 != -1) {
					p[i++] = 97;
					{ int n = p[pos97++];
					  p[i++] = n;
					  while(n--) { p[i++] = p[pos97++]; }
					}
				}

				p[i++] = 53; p[i++] = 1;
				p[i++] = type==1 ? 2 : 5;

				p[i++] = 51; p[i++] = 4;
				p[i++] = 255; p[i++] = 255; p[i++] = 255; p[i++] = 255;

				p[i++] = 54; p[i++] = 4;
				*(uint32_t*)(p+i)=(uint32_t)sc; i+=4;

				p[i++] = 60; p[i++] = 9;
				p[i++] = 'P'; p[i++] = 'X'; p[i++] = 'E'; p[i++] = 'C'; p[i++] = 'l'; p[i++] = 'i'; p[i++] = 'e'; p[i++] = 'n'; p[i++] = 't';

				if(type == 3) {
					p[i++] = 1; p[i++] = 4;
					p[i++] = 255; p[i++] = 0; p[i++] = 0; p[i++] = 0;
					p[i++] = 12; p[i++] = 9;
					p[i++] = 'p'; p[i++] = 'x'; p[i++] = 'e'; p[i++] = 'c'; p[i++] = 'l'; p[i++] = 'i'; p[i++] = 'e'; p[i++] = 'n'; p[i++] = 't';
					p[i++] = 67; m = i++;
					p[i++] = 'p'; p[i++] = 'x'; p[i++] = 'e'; p[i++] = 'l'; p[i++] = 'i'; p[i++] = 'n'; p[i++] = 'u'; p[i++] = 'x'; p[i++] = '.'; p[i++] = '0'; p[i++] = '\0';
					p[m] = i - m - 2;
				}
				
				p[i++] = 43; m = i++;
				{
					if(type == 1) {
						p[i++] = 6; p[i++] = 1; p[i++] = 0;

						p[i++] = 8; p[i++] = 7;
						p[i++] = 0; p[i++] = 0; p[i++] = 1;
						*(uint32_t*)(p+i)=(uint32_t)sc; i+=4;
					}
					p[i++] = 255;
				}
				p[m] = i - m - 2;
				p[i++] = 255;

				{ struct sockaddr_in a = { AF_INET, htons(68), {bc} };
				  sendto(r,p,i,0,(struct sockaddr *)&a,sizeof(a));
				}
			}
		}
	}
}
