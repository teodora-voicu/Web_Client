#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

/* functie ce calculeaza ori get ori delete request, in functie de tip, in loc de doua functii separate
am modificat functia din laborator si parametrii acesteia */
char *compute_get_delete_request(char *host, char *url, char *query_params,
                                char *cookies, char *token, char *type)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *helper = calloc(BUFLEN, sizeof(char));

    /* Step 1: write the method name, URL, request params (if any) and protocol type */
    if (query_params != NULL) {
        sprintf(line, "%s %s?%s HTTP/1.1", type, url, query_params);
    } else {
        sprintf(line, "%s %s HTTP/1.1", type, url);
    }

    compute_message(message, line);

    /* Step 2: add the host */
    strcpy(helper, "Host: ");
    strcat(helper, host);
    compute_message(message, helper);
    for (int i = 0; i < BUFLEN; i++) {
        helper[i] = '\0';
    }


    /* Step 3 (optional): add headers and/or cookies, according to the protocol format */
    if (cookies != NULL) {
        strcpy(helper, "Cookie: ");
        strcat(helper, cookies);
        compute_message(message, helper);
        for (int i = 0; i < BUFLEN; i++) {
            helper[i] = '\0';
        }

    }

    if (token != NULL) {
        strcpy(helper, "Authorization: Bearer ");
        strcat(helper, token);
        compute_message(message, helper);
        for (int i = 0; i < BUFLEN; i++) {
            helper[i] = '\0';
        }

    }

    /* Step 4: add final new line */
    compute_message(message, "");
    free(line);
    free(helper);
    return message;
}

char *compute_post_request(char *host, char *url, char* content_type, char **body_data,
                            int body_data_fields_count, char *cookies, char *token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *helper = calloc(BUFLEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    // Step 2: add the host
    strcpy(helper, "Host: ");
    strcat(helper, host);
    compute_message(message, helper);
    for (int i = 0; i < BUFLEN; i++) {
        helper[i] = '\0';
    }

    /* Step 3: add necessary headers (Content-Type and Content-Length are mandatory)
            in order to write Content-Length you must first compute the message size
    */

    strcpy(helper, "Content-Type: ");
    strcat(helper, content_type);
    compute_message(message, helper);
    for (int i = 0; i < BUFLEN; i++) {
        helper[i] = '\0';
    }

    strcpy(helper, "Content-Length: ");
    char length_str[20];
    sprintf(length_str, "%ld", strlen(*body_data));
    strcat(helper, length_str);
    compute_message(message, helper);
    for (int i = 0; i < BUFLEN; i++) {
        helper[i] = '\0';
    }


    // Step 4 (optional): add cookies
        if (cookies != NULL) {
        strcpy(helper, "Cookie: ");
        strcat(helper, cookies);
        compute_message(message, helper);
        for (int i = 0; i < BUFLEN; i++) {
            helper[i] = '\0';
        }
    }

    if (token != NULL) {
        strcpy(helper, "Authorization: Bearer ");
        strcat(helper, token);
        compute_message(message, helper);
        for (int i = 0; i < BUFLEN; i++) {
            helper[i] = '\0';
        }
    }

    // Step 5: add new line at end of header
    strcat(message, "\r\n");

    // Step 6: add the actual payload data
    compute_message(message, *body_data);

    free(line);
    free(helper);
    return message;
}