/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 */

#ifndef CCNET_RSA_H
#define CCNET_RSA_H

#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/err.h>

 
RSA* private_key_to_pub(RSA *priv);

GString* public_key_to_gstring(const RSA *rsa);
void public_key_append_to_gstring(const RSA *rsa, GString *buf);

RSA* public_key_from_string(char *str);

unsigned char* private_key_decrypt(RSA *key, unsigned char *data,
                                   int len, int *decrypt_len);

unsigned char* public_key_encrypt(RSA *key, unsigned char *data,
                                  int len, int *encrypt_len);


char *id_from_pubkey (RSA *pubkey);

RSA* generate_private_key(u_int bits);


#endif
