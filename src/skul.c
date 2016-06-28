/*
 *    This file is part of Skul.
 *
 *    Copyright 2016, Simone Bossi    <pyno@crypt.coffee>
 *                    Hany Ragab      <_hanyOne@crypt.coffee>
 *                    Alexandro Calo' <ax@crypt.coffee>
 *    Copyright (C) 2014 Cryptcoffee. All rights reserved.
 *
 *    Skull is a PoC to bruteforce the Cryptsetup implementation of
 *    Linux Unified Key Setup (LUKS).
 *
 *    Skul is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License version 2
 *    as published by the Free Software Foundation.
 *
 *    Skul is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Skul.  If not, see <http://www.gnu.org/licenses/>.
 */



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "openssl/evp.h"
#include "openssl/sha.h"
#include "openssl/aes.h"
#include <endian.h>
#include "../lib/skulfs.h"
#include "../lib/utils.h"
#include "../lib/alloclib.h"
#include "../lib/decrypt.h"
#include "../lib/config.h"
#include "../lib/thread.h"
#include "../lib/attacks.h"
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <getopt.h>


int interface_selection(pheader *header,int *slot,int *slot_order, int *tot, 
		int key_sel);

int main(int argc, char **argv){
	
	unsigned char *crypt_disk=NULL;
	char *path, *set, *cfg_path=NULL, *pwlist_path=NULL; 
	pheader header;
	lkey_t encrypted;
	int iv_mode, chain_mode,i,set_len,num,j,c, mod, res=0,threads=0, prompt=1, errflag=0, mode=UNSET,fast=UNSET;
	int slot[8], slot_order[8];
	usrp UP;
	struct timeval t0,t1;
	unsigned long sec;
	FILE *f;

	set = NULL;
	
	/* check arguments */
	if(!(argv[1])){
		print_small_help();
		exit(EXIT_FAILURE);
	}

	while (1) {
		char c, choice;
		int arg;
		
		c = getopt(argc, argv, "hvm:c:t:nf:l:");
		if (c == -1) {
			break;
		}
		switch (c) {

			case 'h':
				print_help();
				return 2;

			case 'v':
				print_version();
				return 3;

			case 'c':
				if(optarg[0]!='-'){
					cfg_path=optarg;
				}else{
					errprint("option requires an argument -- '-%c'\n", c);
					errflag++;
				}
				break;

			case 't':
				arg=atoi(optarg);
				if(arg>0)
					threads=arg;
				else{
					errprint("illegal argument for option -- '-%c'\n", c);
					errflag++;
				}
				break;

			case 'm':
				arg=atoi(optarg);
				if(arg>0 && arg<=4){
					mode=arg;
				}else{
					errprint("illegal argument for option -- '-%c'\n", c);
					errflag++;
				}
				break;

			case 'n':
				prompt=0;
				break;

			case 'f':
				choice=optarg[0];
				switch (choice){
					case 'y':
						fast=1;
						break;
					case 'n':
						fast=0;
						break;
					default:
					errflag++;
				}
				break;

			case 'l':
				if(optarg[0]!='-'){
					pwlist_path=optarg;
				}else{
					errprint("option requires an argument -- '-%c'\n", c);
					errflag++;
				}
				break;

			case ':':
				errprint("option '-%c' requires an argument\n", c);
				break;

			case '?':
			default:
				print_small_help();
				exit(EXIT_FAILURE);
		}
	}

	if(errflag){
		print_small_help();
		exit(EXIT_FAILURE);
	}

	if (optind >= argc) {
		print_small_help();
		exit(EXIT_FAILURE);
	}

	/* last argument must be the disk name */
	path=argv[argc-1];

	/* test pwlist file */
	if(pwlist_path){
		if(!(f=fopen(pwlist_path,"r"))){
			errprint("cannot open %s: %s\n",pwlist_path, strerror(errno));
			errprint("[FATAL] missing password list file\n");
			return 0;
		}else{
			fclose(f);
		}
	}

	/* read configuration file */
	if(!read_cfg(&UP,threads, cfg_path, mode, fast)){
		errprint("[FATAL] missing or invalid configuration file\n");
		exit(EXIT_FAILURE);
	}

	if(!(crypt_disk=calloc(32,sizeof(char)))){
		errprint("calloc error!\n");
		exit(EXIT_FAILURE);
	}
	if(!initfs(&header, &iv_mode, &chain_mode, crypt_disk, path, 
				&encrypted,	slot)){
		exit(EXIT_FAILURE);
	}

	system("clear");
	display_art();

	if(UP.SEL_MOD!=0){
		print_header(&header);
		printf("\nATTACKING KEYSLOTS:\n\n");
		print_keyslot(&header,0);
		printf("\n");
	}

	/* Default */
	for(i=0,c=0;i<8;i++){
		if(slot[i]){
			slot_order[c]=i;
			c++;
		}
	}
	num = c;
	if(UP.SEL_MOD==4)
		mod = interface_selection(&header,slot,slot_order,&num, UP.KEY_SEL);
	else
		mod = UP.SEL_MOD;
	if(mod>4 || mod<=0){
		errprint("Invalid Attack mode selection in configure file\n");
		exit(EXIT_FAILURE);
	}

	OpenSSL_add_all_digests();
	OpenSSL_add_all_algorithms();
	OpenSSL_add_all_ciphers();

	printf("Threads:     %d\n", UP.NUM_THR);
	if(UP.FST_CHK)
		printf("Fast check:  Enabled\n");
	else
		printf("Fast check:  Disabled\n");


	switch(mod){
		
		case 1: /* bruteforce */
			printf("Attack mode: Bruteforce\n\n");
			printf("Min len:     %d characters\n", UP.MIN_LEN);
			printf("Max len:     %d characters\n", UP.MAX_LEN);
			printf("Alphabet:    %d\n\n", UP.ALP_SET);
			if(prompt){
				printf("Press enter to start cracking!");
				getchar();
				printf("\n");
			}

			/* START GLOBAL TIMER */
			gettimeofday(&t0,NULL);

			set = init_set(&set_len,UP.ALP_SET); 
			for(j=0;j<num;j++){
				for(i=UP.MIN_LEN;i<=UP.MAX_LEN;i++){
					if((res=bruteforce(i, set, set_len, &header, iv_mode, 
								chain_mode, &encrypted, crypt_disk,
									slot_order[j],UP.NUM_THR,UP.FST_CHK,UP.PRG_BAR))){
						break;
					}
				}
			}
			break;

		case 2: /* pwlist */
			printf("Attack mode: Password List\n\n");
			if(prompt){
				printf("Press enter to start cracking!");
				getchar();
				printf("\n");
			}

			/* START GLOBAL TIMER */
			gettimeofday(&t0,NULL);

			for(j=0;j<num;j++){
				res=pwlist(&header, iv_mode, chain_mode, &encrypted, crypt_disk,
						slot_order[j],UP.NUM_THR,UP.FST_CHK,UP.PRG_BAR, pwlist_path);
			}
			break;

		case 3:
			/* first call pwlist */
			printf("Attack mode: Password List first, then Bruteforce\n\n");
			printf("Settings for Bruteforce:\n");
			printf("Min len:     %d characters\n", UP.MIN_LEN);
			printf("Max len:     %d characters\n", UP.MAX_LEN);
			printf("Alphabet:    %d\n\n", UP.ALP_SET);
			if(prompt){
				printf("Press enter to start cracking!");
				getchar();
				printf("\n");
			}

			/* START GLOBAL TIMER */
			gettimeofday(&t0,NULL);

			for(j=0;j<num;j++){
				if(!(res=pwlist(&header, iv_mode, chain_mode, &encrypted, crypt_disk,
							slot_order[j],UP.NUM_THR,UP.FST_CHK,UP.PRG_BAR, pwlist_path))){
					/* then call bruteforce */
					set = init_set(&set_len,UP.ALP_SET); 
					for(i=UP.MIN_LEN;i<=UP.MAX_LEN;i++){
						if((res=bruteforce(i, set, set_len, &header, iv_mode, 
									chain_mode, &encrypted, crypt_disk, 
									slot_order[j],UP.NUM_THR,UP.FST_CHK,UP.PRG_BAR))){
							break;
						}
					}
				}
			}
			break;

		default:
			errprint("Invalid Attack Mode\n");
			break;

	}

	/* STOP GLOBAL TIMER */
	gettimeofday(&t1,NULL);
	sec=t1.tv_sec-t0.tv_sec;
	printf("TOTAL TIME: ");
	print_time(sec);

	/* free memory */
	EVP_cleanup();
	free(encrypted.key);
	freeheader(&header);
	free(crypt_disk);
	if(mod==1||mod==3){
		free(set);
	}
	
	if(res==0)
		return 1;

	return 0;
}

int interface_selection(pheader *header,int *slot,int *slot_order,int *tot, int key_sel){

	int continua,s,i,num_slot,n,invalid,mod;
	char *line, ch;

	invalid=0;
	continua=1;
	line=NULL;
	while(continua){

		system("clear");
		display_art_nosleep();
		print_header(header);
		printf("\nACTIVE KEYSLOTS:\n\n");
		num_slot=0;
		for(i=0;i<8;i++){
			if(slot[i]){
				slot_order[num_slot]=i;
				num_slot++;
				print_keyslot(header,i);
				printf("\n");
			}
		}

		if(invalid)
			printf("Invalid selection!");
		invalid=0;
		if(key_sel){
			printf("\nSelect keyslots to attack in the desired order (0,1,2):\n$ ");
			line = readline(stdin, &n);
			if(n<=0)
				continue;
			for(i=0;i<n;i+=2){
				sscanf(line+i,"%d,",&s);
				if(s<0 || s>7){
					invalid=1;
					break;
				}
				if(!slot[s]){
					invalid=1;
					break;
				}
			}
			if(invalid)
				continue;
			for(i=0;i<=n;i+=2){
				num_slot=0;
				sscanf(line+i,"%d",&s);
				slot_order[num_slot]=s;
				num_slot++;
			}
		}
		continua=0;
	}

	continua=1;
	while(continua){

		if(key_sel){
			system("clear");
			display_art_nosleep();
			print_header(header);
			printf("\nSELECTED KEYSLOTS: %d\n\n", num_slot);
			for(i=0;i<num_slot;i++){
				print_keyslot(header,slot_order[i]);
				printf("\n");
			}
		}
		else{
			system("clear");
			display_art_nosleep();
			print_header(header);
			printf("\nACTIVE KEYSLOTS:\n\n");
			for(i=0;i<8;i++){
				if(slot[i]){
					print_keyslot(header,i);
					printf("\n");
				}
			}

		
		}

		if(invalid)
			printf("Invalid selection!");
		invalid=0;
		printf("\nSelect attack mode:\n1) bruteforce\n2) password list\n3) password list and bruteforce\n$ ");
		ch = getchar();
		mod = atoi(&ch);
		if((mod!=3) && (mod!=1) && (mod!=2)){
			invalid=1;
			continue;
		}
		continua=0;
	}

	*tot=num_slot;
	free(line);
	return mod;
}
