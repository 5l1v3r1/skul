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



#include "thread.h"
#include "../src/skul.h"
#include "errno.h"
#include <string.h>
#include "utils.h"
#include <sys/time.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

char *init_set(int *set_len, int id_set){

	char *set, c;
	int i;

	set = NULL;
	switch (id_set){
		case 1:
			c='a';
			*set_len = 26;
			set = calloc(*set_len, sizeof(char));
			for(i=0;i<*set_len;i++){
				set[i] = c++;
			}
			break;
		case 2:
			c='A';
			*set_len = 26;		
			set = calloc(*set_len, sizeof(char));
			for(i=0;i<*set_len;i++){
				set[i]=c++;
			}
			break;
		case 3:
			c='0';
			*set_len = 10;
			set = calloc(*set_len, sizeof(char));
			for(i=0;i<*set_len;i++){
				set[i]=c++;
			}
			break;
		case 4:
			*set_len = 52;
			set = calloc(*set_len, sizeof(char));
			c='A';
			for(i=0;i<26;i++){
					set[i]=c++;
			}
			c='a';
			for(i=26;i<52;i++){
				set[i]=c++;
			}
			break;
		case 5:
			*set_len = 36;
			c='0';
			set = calloc(*set_len, sizeof(char));
			for(i=0;i<=9;i++){
					set[i]=c++;
			}
			c='a';
			for(i=10;i<36;i++){
				set[i]=c++;
			}
			break;
		case 6:
			*set_len = 62;
			c='0';
			set = calloc(*set_len, sizeof(char));
			for(i=0;i<=9;i++){
					set[i]=c++;
			}
			c='A';
			for(i=10;i<36;i++){
					set[i]=c++;
			}
			c='a';
			for(i=36;i<62;i++){
				set[i]=c++;
			}
			break;
		case 7:
			*set_len=95;
			set = calloc(*set_len, sizeof(char));
			c=' ';
			for(i=0;i<95;i++){
				set[i]=c++;
			}
			break;
	}

	return set;
}

int bruteforce(int len, char *set, int set_len, SKUL_CTX *ctx){

	int j,jpt,jptr,lpt,lptr/*,num,comb*/,tot_comb,*progress,reminder,start,tot=0,found=1;
	thforce_data *arg;
	pthread_t *threads;
	pthread_attr_t attr;
	char *win_pwd;
	void *(*force)(void *);
	int (*control)(int,int,pthread_t *,int, pheader *, int *, char *, int,int);
	unsigned long sec;
	struct timeval t0,t1;

	if (len == 1)
		printf("Bruteforce: %d char\n", len);
	else
		printf("Bruteforce: %d chars\n", len);

	if((arg = calloc(ctx->UP.NUM_THR, sizeof(thforce_data)))==NULL){
		errprint("malloc error!\n");
		return 0;
	}
	if((threads = calloc(ctx->UP.NUM_THR, sizeof(pthread_t)))==NULL){
		errprint("malloc error!\n");
		return 0;
	}
	if((win_pwd = calloc(len,sizeof(char)))==NULL){
		errprint("malloc error!\n");
		return 0;
	}
	if((progress = calloc(ctx->UP.NUM_THR, sizeof(int)))==NULL){
		errprint("malloc error!\n");
		return 0;
	}


	/* attributes initialization */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);

	/* arguments initialization */
	tot_comb = pow(set_len,len);
	lpt = set_len/ctx->UP.NUM_THR;
	lptr = lpt+1;
	reminder = set_len%ctx->UP.NUM_THR;
	jpt = pow(set_len,len-1)*lpt;
	jptr = pow(set_len,len-1)*(lptr);
	start=0;

	/* threads with reminder */
	for(j=0;j<reminder;j++){
		start = j*lptr;
		if(!thforce_datainit(&(arg[j]), ctx, j, start, lptr, jptr, len, set_len, 
					set, &progress[j], win_pwd)){
			errprint("thforce_datainit error!\n");
			return 0;
		}
	}

	/* threads without reminder */
	for(j=reminder;j<ctx->UP.NUM_THR;j++){
		start = reminder*lptr + (j-reminder)*lpt;
		if(!thforce_datainit(&(arg[j]), ctx, j, start, lpt, jpt, len, set_len, 
					set, &progress[j], win_pwd)){
			errprint("thforce_datainit error!\n");
			return 0;
		}
	}
#ifdef TESTING
	force = test_force;
	control = test_control;
#else
	force = th_force;
	control = th_control;
#endif

	/* START TIME */
	gettimeofday(&t0,NULL);

	/* threads creation */
	for(j=0;j<ctx->UP.NUM_THR;j++){
		if( pthread_create(&threads[j],&attr, force, (void *)&arg[j]) ){
			errprint("pthread_create error!\n");
			return 0;
		}
	}

	/* TODO: make independent from LUKS */
	if(!control(len, tot_comb, threads, ctx->UP.NUM_THR, &(ctx->luks->header), progress, win_pwd, 
				ctx->cur_pwd, ctx->UP.PRG_BAR)){
		printf("Password not found\n");
		found = 0;
	}

#ifdef TESTING

	for(j=0;j<ctx->UP.NUM_THR;j++){
		pthread_join(threads[j],NULL);
	}

#endif

	/* END TIME */
	gettimeofday(&t1,NULL);
	sec=t1.tv_sec-t0.tv_sec;
	tot=0;
	if(!ctx->UP.PRG_BAR){
		for(j=0;j<ctx->UP.NUM_THR;j++){
			tot+=progress[j];
		}
		printf("Tried: %d passwords - ",tot);
	}
	if(!found){
		printf("Time: ");
		print_time(sec);
		printf("\n");
	}


	pthread_attr_destroy(&attr);
	free(win_pwd);
	free(threads);

	return found;
}

int pwlist(SKUL_CTX *ctx){

	char **list, c, *win_pwd;
	int i=0,j, count=0,jforth,max_l=0,cur_l,lastj, *progress, tot=0, found=1;
	FILE *f;
	thlist_data *arg;
	pthread_t *threads;
	pthread_attr_t attr;
	void *(*lst)(void *);
	int (*control)(int,int,pthread_t *,int, pheader *, int *, char *, int, int);
	unsigned long sec=0;
	struct timeval t0,t1;

	if(ctx->pwlist_path){
		if(!(f=fopen(ctx->pwlist_path,"r"))){
			errprint("cannot open %s: %s\n",ctx->pwlist_path, strerror(errno));
			return 0;
		}
	}else{
		if(!(f=fopen("conf/pwlist","r"))){
			perror("fopen");
			errprint("fopen error\n");
			return 0;
		}
	}
	while((c=fgetc(f))!=EOF){
		if(c=='\n')
			count++;
	}
	fseek(f,0,SEEK_SET);
	if((list=calloc(count+2,sizeof(char *)))==NULL){
		errprint("malloc error!\n");
		return 0;
	}

	while((list[i]=readline(f,&cur_l))){
		if(cur_l>max_l){
			max_l = cur_l;
		}
		i++;
	}
	fclose(f);

	printf("Password list: %d passwords\n", count);

	if((win_pwd = calloc(max_l,sizeof(char)))==NULL){
		errprint("malloc error!\n");
		return 0;
	}
	if((arg = calloc(ctx->UP.NUM_THR, sizeof(thlist_data)))==NULL){
		errprint("malloc error!\n");
		return 0;
	}
	if((threads = calloc(ctx->UP.NUM_THR, sizeof(pthread_t)))==NULL){
		errprint("malloc error!\n");
		return 0;
	}
	if((progress = calloc(ctx->UP.NUM_THR, sizeof(int)))==NULL){
		errprint("malloc error!\n");
		return 0;
	}
	
	/* attributes initialization */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);

//	int *addr = calloc(80000,sizeof(char));
//	printf("%016x\n", &addr);
//	printf("-----------\n");
//	fflush(stdout);


	/* arguments initialization */
	jforth = count/ctx->UP.NUM_THR;
	for(j=0;j<ctx->UP.NUM_THR-1;j++){
		if(!thlist_datainit(&(arg[j]), ctx, j, list+(j*jforth), jforth,
					max_l, &progress[j],win_pwd)){
			errprint("thlist_datainit error!\n");
			return 0;
		}
	}
#ifdef TESTING
	lst = test_list;
	control = test_control;
#else
	lst = th_list;
	control = th_control;
#endif

	/* START TIME */
	gettimeofday(&t0,NULL);

	/* threads creation */
	for(j=0;j<ctx->UP.NUM_THR-1;j++){
		if( pthread_create(&threads[j],&attr, lst, (void *)&arg[j]) ){
			errprint("pthread_create error!\n");
			return 0;
		}
	}

	/* 
	 * last thread creation (we need to do this apart because of the
	 * approssimation of integer division)
	 */
	lastj = count - (jforth *(ctx->UP.NUM_THR-1));
	if(!thlist_datainit(&(arg[j]), ctx, j, list+(j*jforth), lastj,
				max_l, &progress[j],win_pwd)){
		errprint("thlistdata_init error!\n");
		return 0;
	}
	if( pthread_create(&threads[ctx->UP.NUM_THR-1],&attr, lst, (void *)&arg[j]) ){
			errprint("pthread_create error!\n");
			return 0;
		}

	for(i=0;i<count;i++){
		free(list[i]);
	}
	free(list);

	/*TODO: make independent from luks*/
	if(!control(max_l,count, threads, ctx->UP.NUM_THR, &(ctx->luks->header), progress, win_pwd,
				ctx->cur_pwd, ctx->UP.PRG_BAR)){
		printf("Password not found\n");
		found = 0;
	}
#ifdef TEST

	for(j=0;j<ctx->UP.NUM_THR;j++){
		pthread_join(threads[j],NULL);
	}

#endif

	/* END TIME */
	gettimeofday(&t1,NULL);
	sec=t1.tv_sec-t0.tv_sec;
	tot=0;
	if(!ctx->UP.PRG_BAR){
		for(j=0;j<ctx->UP.NUM_THR;j++){
			tot+=progress[j];
		}
		printf("Tried: %d passwords -  ",tot);
		fflush(stdout);
	}
	if(!found){
		printf("Time: ");
		print_time(sec);
		printf("\n");
	}


	pthread_attr_destroy(&attr);

	free(win_pwd);
	return found;
}
