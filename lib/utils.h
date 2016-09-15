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



#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>
#include <stdio.h>
void print_format(unsigned long val);
void print_time(unsigned long sec);
int errprint(const char *format, ...);
int debug_print(const char *format, ...);
int warn_print(const char *format, ...);
void dbgprintkey(uint8_t *key, int len, char *name);
uint32_t l2bEndian(uint32_t num);
void display_art();
void display_art_nosleep();
void print_help();
void print_version();
void print_small_help();
char *readline(FILE *f, int *max_l);
int uint16read(uint16_t *res, FILE *stream);
int uint32read(uint32_t *res, FILE *stream);

#endif
