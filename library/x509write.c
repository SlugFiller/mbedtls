/*
 * X509 buffer writing functionality
 *
 *  Copyright (C) 2006-2012, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "polarssl/config.h"

#if defined(POLARSSL_X509_WRITE_C)

#include "polarssl/asn1write.h"
#include "polarssl/x509write.h"
#include "polarssl/x509.h"
#include "polarssl/sha1.h"

int x509_write_pubkey_der( unsigned char *buf, size_t size, rsa_context *rsa )
{
    int ret;
    unsigned char *c;
    size_t len = 0;

    c = buf + size - 1;

    ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->E ) );
    ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->N ) );

    ASN1_CHK_ADD( len, asn1_write_len( &c, buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, buf, ASN1_CONSTRUCTED | ASN1_SEQUENCE ) );

    if( c - buf < 1 )
        return( POLARSSL_ERR_ASN1_BUF_TOO_SMALL );

    *--c = 0;
    len += 1;

    ASN1_CHK_ADD( len, asn1_write_len( &c, buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, buf, ASN1_BIT_STRING ) );

    ASN1_CHK_ADD( len, asn1_write_algorithm_identifier( &c, buf, OID_PKCS1_RSA ) );

    ASN1_CHK_ADD( len, asn1_write_len( &c, buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, buf, ASN1_CONSTRUCTED | ASN1_SEQUENCE ) );

    return( len );
}

int x509_write_key_der( unsigned char *buf, size_t size, rsa_context *rsa )
{
    int ret;
    unsigned char *c;
    size_t len = 0;

    c = buf + size - 1;

    ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->QP ) );
    ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->DQ ) );
    ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->DP ) );
    ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->Q ) );
    ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->P ) );
    ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->D ) );
    ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->E ) );
    ASN1_CHK_ADD( len, asn1_write_mpi( &c, buf, &rsa->N ) );
    ASN1_CHK_ADD( len, asn1_write_int( &c, buf, 0 ) );

    ASN1_CHK_ADD( len, asn1_write_len( &c, buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, buf, ASN1_CONSTRUCTED | ASN1_SEQUENCE ) );

    // TODO: Make NON RSA Specific variant later on
/*    *--c = 0;
    len += 1;

    len += asn1_write_len( &c, len);
    len += asn1_write_tag( &c, ASN1_BIT_STRING );

    len += asn1_write_oid( &c, OID_PKCS1_RSA );

    len += asn1_write_int( &c, 0 );

    len += asn1_write_len( &c, len);
    len += asn1_write_tag( &c, ASN1_CONSTRUCTED | ASN1_SEQUENCE );*/

/*    for(i = 0; i < len; ++i)
    {
        if (i % 16 == 0 ) printf("\n");
        printf("%02x ", c[i]);
    }
    printf("\n");*/

    return( len );
}

int x509_write_name( unsigned char **p, unsigned char *start, char *oid,
                     char *name )
{
    int ret;
    size_t string_len = 0;
    size_t oid_len = 0;
    size_t len = 0;

    // Write PrintableString
    //
    ASN1_CHK_ADD( string_len, asn1_write_printable_string( p, start, name ) );

    // Write OID
    //
    ASN1_CHK_ADD( oid_len, asn1_write_oid( p, start, oid ) );

    len = oid_len + string_len;
    ASN1_CHK_ADD( len, asn1_write_len( p, start, oid_len + string_len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( p, start, ASN1_CONSTRUCTED | ASN1_SEQUENCE ) );

    ASN1_CHK_ADD( len, asn1_write_len( p, start, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( p, start, ASN1_CONSTRUCTED | ASN1_SET ) );

    return( len );
}

int x509_write_sig( unsigned char **p, unsigned char *start, char *oid,
                    unsigned char *sig, size_t size )
{
    int ret;
    size_t len = 0;

    if( *p - start < (int) size + 1 )
        return( POLARSSL_ERR_ASN1_BUF_TOO_SMALL );

    len = size;
    (*p) -= len;
    memcpy( *p, sig, len );

    *--(*p) = 0;
    len += 1;

    ASN1_CHK_ADD( len, asn1_write_len( p, start, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( p, start, ASN1_BIT_STRING ) );

    // Write OID
    //
    ASN1_CHK_ADD( len, asn1_write_algorithm_identifier( p, start, oid ) );

    return( len );
}

int x509_write_cert_req( unsigned char *buf, size_t size, rsa_context *rsa,
                         x509_req_name *req_name )
{
    int ret;
    unsigned char *c, *c2;
    unsigned char hash[20];
    unsigned char sig[512];
    unsigned char tmp_buf[2048];
    size_t sub_len = 0, pub_len = 0, sig_len = 0;
    size_t len = 0;
    x509_req_name *cur = req_name;

    c = tmp_buf + 2048 - 1;

    ASN1_CHK_ADD( len, asn1_write_len( &c, tmp_buf, 0 ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, tmp_buf, ASN1_CONSTRUCTED | ASN1_CONTEXT_SPECIFIC ) );

    ASN1_CHK_ADD( pub_len, asn1_write_mpi( &c, tmp_buf, &rsa->E ) );
    ASN1_CHK_ADD( pub_len, asn1_write_mpi( &c, tmp_buf, &rsa->N ) );

    ASN1_CHK_ADD( pub_len, asn1_write_len( &c, tmp_buf, pub_len ) );
    ASN1_CHK_ADD( pub_len, asn1_write_tag( &c, tmp_buf, ASN1_CONSTRUCTED | ASN1_SEQUENCE ) );

    if( c - tmp_buf < 1 )
        return( POLARSSL_ERR_ASN1_BUF_TOO_SMALL );

    *--c = 0;
    pub_len += 1;

    ASN1_CHK_ADD( pub_len, asn1_write_len( &c, tmp_buf, pub_len ) );
    ASN1_CHK_ADD( pub_len, asn1_write_tag( &c, tmp_buf, ASN1_BIT_STRING ) );

    ASN1_CHK_ADD( pub_len, asn1_write_algorithm_identifier( &c, tmp_buf, OID_PKCS1_RSA ) );

    len += pub_len;
    ASN1_CHK_ADD( len, asn1_write_len( &c, tmp_buf, pub_len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, tmp_buf, ASN1_CONSTRUCTED | ASN1_SEQUENCE ) );

    while( cur != NULL )
    {
        ASN1_CHK_ADD( sub_len, x509_write_name( &c, tmp_buf, cur->oid, cur->name ) );
        
        cur = cur->next;
    }

    len += sub_len;
    ASN1_CHK_ADD( len, asn1_write_len( &c, tmp_buf, sub_len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, tmp_buf, ASN1_CONSTRUCTED | ASN1_SEQUENCE ) );

    ASN1_CHK_ADD( len, asn1_write_int( &c, tmp_buf, 0 ) );

    ASN1_CHK_ADD( len, asn1_write_len( &c, tmp_buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c, tmp_buf, ASN1_CONSTRUCTED | ASN1_SEQUENCE ) );
    
    sha1( c, len, hash );
    rsa_pkcs1_sign( rsa, NULL, NULL, RSA_PRIVATE, SIG_RSA_SHA1, 0, hash, sig );

    c2 = buf + size - 1;
    ASN1_CHK_ADD( sig_len, x509_write_sig( &c2, buf, OID_PKCS1_SHA1, sig, rsa->len ) );
    
    c2 -= len;
    memcpy( c2, c, len ); 
    
    len += sig_len;
    ASN1_CHK_ADD( len, asn1_write_len( &c2, buf, len ) );
    ASN1_CHK_ADD( len, asn1_write_tag( &c2, buf, ASN1_CONSTRUCTED | ASN1_SEQUENCE ) );

    return( len );
}

#endif
