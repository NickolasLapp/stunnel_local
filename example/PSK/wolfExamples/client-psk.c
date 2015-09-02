
/* client-psk.c
 *
 * Copyright (C) 2006-2015 wolfSSL Inc.
 *
 * This file is part of wolfSSL. (formerly known as CyaSSL)
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 **/

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <errno.h>
#include    <arpa/inet.h>
#include    <signal.h>
#include    <unistd.h>
#include    <wolfssl/ssl.h>  /* must include this to use wolfSSL security */

#define     MAXLINE 256      /* max text line length */
#define     SERV_PORT 11112  /* default port*/

/*
 *psk client set up.
 */
static inline unsigned int my_Psk_Client_Cb(WOLFSSL* ssl, const char* hint,
        char* identity, unsigned int id_max_len, unsigned char* key, 
        unsigned int key_max_len)
{
    (void)ssl;
    (void)hint;
    (void)key_max_len;
    char* keyToBe = "1a2b3c4d1a2b3c4d1a2b3c4d1a2b3c4d1a2b3c4d";
    int   keyLen  = strlen(keyToBe);

    /* identity is OpenSSL testing default for openssl s_client, keep same*/
    strncpy(identity, "Client_identity", id_max_len);
    strncpy((char*)key, keyToBe, keyLen);

    return keyLen;
}

/*
 * this function will send the inputted string to the server and then 
 * recieve the string from the server outputing it to the termial
 */ 
int SendReceive(WOLFSSL* ssl)
{
    char sendline[MAXLINE]="Happy Tuesday (It is really Monday)"; /* string to send to the server */
    char recvline[MAXLINE]; /* string received from the server */
        
	/* write string to the server */
    int temp = wolfSSL_write(ssl, sendline, MAXLINE);
	if ( temp!= sizeof(sendline)) {
		printf("Write Error to Server\t%d\n", temp);
		return 1;
    }
        
	/* flags if the Server stopped before the client could end */
    if (wolfSSL_read(ssl, recvline, MAXLINE) < 0 ) {
    	printf("Client: Server Terminated Prematurely!\n");
        return 1;
    }

    /* show message from the server */
	printf("Server Message: %s\n", recvline);
        	
	return 0;
}

int main(int argc, char **argv)
{
    wolfSSL_Debugging_ON();
    int ret, sockfd;
    WOLFSSL* ssl;
    WOLFSSL_CTX* ctx;
    struct sockaddr_in servaddr;;

    /* must include an ip address of this will flag */
    if (argc != 2) {
        printf("Usage: tcpClient <IPaddress>\n");
        return 1;
    }
    
    wolfSSL_Init();  /* initialize wolfSSL */
    
    /* create and initialize WOLFSSL_CTX structure */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "SSL_CTX_new error.\n");
        return 1;
    }
                
    /* create a stream socket using tcp,internet protocal IPv4,
     * full-duplex stream */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    /* places n zero-valued bytes in the address servaddr */
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);

    /* converts IPv4 addresses from text to binary form */
    ret = inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    if (ret != 1) {
        printf("inet_pton error\n");    
		return 1;
    }
    
    /* set up pre shared keys */
    wolfSSL_CTX_set_psk_client_callback(ctx, my_Psk_Client_Cb);
    /* attempts to make a connection on a socket */
    ret = connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    
    if (ret != 0) {
        printf("Connection Error\n");
        return 1;
    }
    
    /* creat wolfssl object after each tcp connct */
    if ( (ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "wolfSSL_new error.\n");
        return 1;
    }
	
    /* associate the file descriptor with the session */
    ret = wolfSSL_set_fd(ssl, sockfd);
	
    if (ret != SSL_SUCCESS){
        return 1;
    }
	
    /* takes inputting string and outputs it to the server */
	ret = SendReceive(ssl);
	if(ret != 0){
		return 1;
	}

    ret = wolfSSL_shutdown(ssl);

    /* cleanup */
    wolfSSL_free(ssl);

    /* when completely done using SSL/TLS, free the 
     * wolfssl_ctx object */
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    /* exit client */
    return ret;
}
