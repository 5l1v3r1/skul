#ifndef _LUKS_H_
#define _LUKS_H_

#include "../../src/skul.h"
#include <stdint.h>
#include <stdlib.h>

#define LUKS_DIGESTSIZE 20
#define LUKS_SALTSIZE 32
#define LUKS_NUMKEYS 8
#define LUKS_KEY_DISABLED 0x0000DEAD
#define LUKS_KEY_ENABLED 0x00AC71F3
#define LUKS_STRIPES 4000
#define SECTOR_SIZE 512

typedef struct key{
	unsigned char *key;
	size_t keylen;
}lkey_t;

typedef struct key_slot{
	uint32_t active;
	uint32_t iterations;
	unsigned char *salt;
	uint32_t key_material_offset;
	uint32_t stripes;
}keyslot_t;

typedef struct luks_header{
	unsigned char *magic;
	uint16_t version;
	unsigned char *cipher_name;
	unsigned char *cipher_mode;
	unsigned char *hash_spec;
	uint32_t payload_offset;
	uint32_t key_bytes;
	unsigned char *mk_digest;
	unsigned char *mk_digest_salt;
	uint32_t mk_digest_iter;
	unsigned char *uuid;
	keyslot_t keyslot[LUKS_NUMKEYS];

	/* functions */
	int (*pbkdf2_function)();
	int (*decrypt_masterkey)();
}pheader;


typedef struct luks_ctx{

	int iv_mode;
	int chain_mode;
	int pbk_hash;
	int slot[8];
	unsigned char *crypt_disk;
	lkey_t encrypted;
	pheader header;

}LUKS_CTX;

void LUKS_print_header(LUKS_CTX *ctx);
int LUKS_init(LUKS_CTX *ctx, int pwd_default, int *num_pwds, int *pwd_ord, char *path,
		usrp UP, int *attack_mode);
int LUKS_CTXcpy(LUKS_CTX *dest, LUKS_CTX *surc);
void LUKS_clean(LUKS_CTX *ctx);

#endif
