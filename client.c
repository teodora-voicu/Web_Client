#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

#define HOST "34.246.184.49"
#define LEN 5000
#define PORT 8080

char *message;

/* functie helper pentru comenzile de register si login */
char *process_login_or_register(size_t buffer_length, int socket_tcp, char *type)
{
    JSON_Value *json_value = json_value_init_object();
    JSON_Object *json_object = json_value_get_object(json_value);
    
    // setare access route si payload type
    char *access_route = (char*)calloc(BUFLEN, sizeof(char));
    strcpy(access_route, "/api/v1/tema/auth/");
    strcat(access_route, type);
    char *payload_type = (char*)calloc(BUFLEN, sizeof(char));
    strcpy(payload_type, "application/json");

    // credentialele de la user
    char *username = (char*)calloc(BUFLEN, sizeof(char));
    char *password = (char*)calloc(BUFLEN, sizeof(char));

    printf("username=");
    fgets(username, BUFLEN, stdin);
    username[strlen(username) - 1] = '\0';


    printf("password=");
    fgets(password, BUFLEN, stdin);
    password[strlen(password) - 1] = '\0';

    // construim obiectul JSON
    json_object_set_string(json_object, "username", username);
    json_object_set_string(json_object, "password", password);
    char *data = json_serialize_to_string(json_value);

    message = compute_post_request(HOST, access_route, payload_type, &data, strlen(data), NULL, NULL);
    free(data);

    // trimitem mesajul la server
    send_to_server(socket_tcp, message);

    free(message);
    free(access_route);
    free(payload_type);
    free(username);
    free(password);
    json_value_free(json_value);

    // primim raspunsul de la server
    char *response = receive_from_server(socket_tcp);

    return response;
}

/* functie helper pentru a gasi un substring intr un string */
int contains_str(const char *str, const char *substr) {
    const char *p = str;
    size_t len = strlen(substr);

    while (*p) {
        if (strncmp(p, substr, len) == 0) {
            return 1;
        }
        p++;
    }
    return 0;
}

/* functie helper pentru cautarea unui caracter intr un string */
char* find_char(const char *str, char c) {
    while (*str) {
        if (*str == c) {
            return (char*)str;
        }
        str++;
    }
    return NULL;
}

void register_user(size_t buffer_length) {

    int socket_tcp = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *response;
    response = process_login_or_register(buffer_length, socket_tcp, "register");

    // verificam erorile
    if (contains_str(response, "Bad Request")) {
        printf("ERROR - username already exits\n");
    } else {
        printf("200 - OK - user registered with SUCCESS\n");
    }

    free(response);
    close(socket_tcp);
}

void login_user(size_t buffer_length, char **session_cookie) {

    int socket_tcp = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *response = process_login_or_register(buffer_length, socket_tcp, "login");

    // dacă un utilizator este deja logat - eroare
    if (*session_cookie != NULL) {
        printf("ERROR - user logged in\n");
        return;
    }
    //daca avem credentiale gresite -eroare 
    if (contains_str(response, "Bad Request")) {
        printf("ERROR - wrong credentials\n");
    } else {
        // salvează cookie ul din response ul serverului
        *session_cookie = calloc(BUFLEN, sizeof(char));
        char *start = strstr(response, "connect.sid=");

         if (start != NULL) {
            char *end = start;
            while (*end && *end != ';') {
                end++;
            }
            *session_cookie = (char *)calloc(end - start + 1, sizeof(char));
            memcpy(*session_cookie, start, end - start);
        }
        printf("200 - OK - user logged in with SUCCESS\n");
    }

    close(socket_tcp);
    free(response);
}

void enter_library(char *session_cookie, char **token) {

    if (session_cookie == NULL) {
        printf("ERROR - no user logged in\n");
        return;
    }
    int socket_tcp = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *access_route = (char*)calloc(BUFLEN, sizeof(char));
    strcpy(access_route, "/api/v1/tema/library/access");

    // get requestul
    message = compute_get_delete_request(HOST, access_route, NULL, session_cookie, NULL, "GET");

    // trimitem mesajul la server
    send_to_server(socket_tcp, message);
    char *response = receive_from_server(socket_tcp);
    free(message);
    free(access_route);

    // verificam erorile
    if (contains_str(response, "error")) {
        printf("ERROR - couldn't reach library\n");
        free(response);
        close(socket_tcp);
        return;
    }

    // obtinem json ul de la raspunsul serverului
    *token = (char*)calloc(BUFLEN, sizeof(char));
    char *start = find_char(response, '{');
    if (start != NULL) {
        char *end = find_char(start, '}');
        if (end != NULL) {
            memcpy(*token, start, end - start + 1);
        }
    }

    // construim obiectul json pt valoarea tokenului
    JSON_Value *value_string = json_parse_string(*token);
    JSON_Object *object_string = json_value_get_object(value_string);
    const char *getter = json_object_get_string(object_string, "token");
    memset(*token, 0, BUFLEN);
    strcpy(*token, getter);

    printf("200 - OK - entered library with SUCCESS\n");

    json_value_free(value_string);
    free(response);
    close(socket_tcp);
}

void get_books(char *session_cookie, char *token) {

    if (session_cookie == NULL) {
        printf("ERROR - no user logged in\n");
        return;
    }

    int socket_tcp = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *access_route = (char*)calloc(BUFLEN, sizeof(char));
    strcpy(access_route, "/api/v1/tema/library/books");

    message = compute_get_delete_request(HOST, access_route, NULL, session_cookie, token, "GET");

    send_to_server(socket_tcp, message);
    char *response = receive_from_server(socket_tcp);
    free(message);
    free(access_route);

    if (contains_str(response, "error")) {
        printf("ERROR - no library access\n");
        free(response);
        close(socket_tcp);
        return;
    }

    // initializam obiectul json pt a lua array ul din response
    char *start = find_char(response, '[');
    if (start == NULL) {
        free(response);
        close(socket_tcp);
        return;
    }
    JSON_Value *json_value = json_parse_string(start);
    
    JSON_Array *books = json_value_get_array(json_value);
    size_t book_count = json_array_get_count(books);

    if (book_count == 0) {
        printf("ERROR - no books yet\n");
    } else {
        printf("ID - TITLE\n");
        for (size_t i = 0; i < book_count; i++) {
            JSON_Object *book = json_array_get_object(books, i);
            int book_id = (int)json_object_get_number(book, "id");
            const char *book_title = json_object_get_string(book, "title");
            printf("id: %d - title: %s\n", book_id, book_title);
        }
    }


    json_value_free(json_value);
    free(response);
    close(socket_tcp);
}

/* functie helper pentru a afisa info despre carte */
void display_book_info(JSON_Object *book) {
    int book_id = (int)json_object_get_number(book, "id");
    const char *book_title = json_object_get_string(book, "title");
    const char *book_author = json_object_get_string(book, "author");
    const char *book_publisher = json_object_get_string(book, "publisher");
    const char *book_genre = json_object_get_string(book, "genre");
    int book_page_count = (int)json_object_get_number(book, "page_count");

    printf("id: %d\n", book_id);
    printf("title: %s\n", book_title);
    printf("author: %s\n", book_author);
    printf("publisher: %s\n", book_publisher);
    printf("genre: %s\n", book_genre);
    printf("page_count: %d\n", book_page_count);
}

void get_book(size_t buffer_length, char *session_cookie, char *token) {

    if (session_cookie == NULL) {
        printf("ERROR - no user logged in\n");
        return;
    }

    int socket_tcp = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    char *username = (char*)calloc(BUFLEN, sizeof(char));
    printf("id=");
    fgets(username, BUFLEN, stdin);
    username[strlen(username) - 1] = '\0';
    char *access_route = (char*)calloc(BUFLEN, sizeof(char));
    strcpy(access_route, "/api/v1/tema/library/books/");
    strcat(access_route, username);

    message = compute_get_delete_request(HOST, access_route, NULL, session_cookie, token, "GET");

    send_to_server(socket_tcp, message);
    char *response = receive_from_server(socket_tcp);
    free(message);
    free(access_route);
    free(username);

    // case in care nu avem acel id
    if (contains_str(response, "error")) {
        printf("ERROR - no book with id %s\n", username);
        free(response);
        close(socket_tcp);
        return;
    }

    // initializam obiectul json
    char *start = find_char(response, '{');
    if (start == NULL) {
        free(response);
        close(socket_tcp);
        return;
    }

    JSON_Value *json_value = json_parse_string(start);

    // afisam toate informatiile despre carte
    JSON_Object *book = json_value_get_object(json_value);
    display_book_info(book);

    free(response);
    json_value_free(json_value);
    close(socket_tcp);
}

void delete_book(size_t buffer_length, char *session_cookie, char *token) {

    if (session_cookie == NULL) {
        printf("ERROR - no user logged in\n");
        return;
    }

    int socket_tcp = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    char *username = (char*)calloc(BUFLEN, sizeof(char));
    printf("id=");
    fgets(username, BUFLEN, stdin);
    username[strlen(username) - 1] = '\0';
    char *access_route = (char*)calloc(BUFLEN, sizeof(char));
    strcpy(access_route, "/api/v1/tema/library/books/");
    strcat(access_route, username);

    message = compute_get_delete_request(HOST, access_route, NULL, session_cookie, token, "DELETE");

    send_to_server(socket_tcp, message);
    char *response = receive_from_server(socket_tcp);
    free(message);
    free(access_route);
    free(username);

    if (contains_str(response, "error")) {
        printf("ERROR - no book with id %s\n", username);
        free(response);
        close(socket_tcp);
        return;
    }
    printf("200 - OK - book deleted with SUCCESS\n");

    free(response);
    close(socket_tcp);
}

/* functie helper care verifica daca un string este numar */
int verify_no(char *s)
{
    char *p = s;
    if (*p == '\0') {
        return 0;
    }
    //prima cifra nu poate fi 0
    if (*p == '0') {
        return 0;
    }
    while (*p != '\0') {
        if (*p < '0' || *p > '9') {
            return 0;
        }
        p++;
    }
    return 1;
}

void add_book(size_t buffer_length, char *session_cookie, char *token) {
    
    int socket_tcp = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    // setarea rutei de access
    char *access_route = (char*)calloc(BUFLEN, sizeof(char));
    strcpy(access_route, "/api/v1/tema/library/books");

    // setarea tipului de payload
    char *payload_type = (char*)calloc(BUFLEN, sizeof(char));
    strcpy(payload_type, "application/json");

    // initializam obiectul json
    JSON_Value *json_value = json_value_init_object();
    JSON_Object *book_object = json_value_get_object(json_value);

    // citim inputul si il salvam in json
    char *all_use_buffer = (char*)calloc(BUFLEN, sizeof(char));
    printf("title=");
    fgets(all_use_buffer, BUFLEN, stdin);
    all_use_buffer[strlen(all_use_buffer) - 1] = '\0';
    json_object_set_string(book_object, "title", all_use_buffer);

    free(all_use_buffer);
    all_use_buffer = (char*)calloc(BUFLEN, sizeof(char));

    printf("author=");
    fgets(all_use_buffer, BUFLEN, stdin);
    all_use_buffer[strlen(all_use_buffer) - 1] = '\0';
    json_object_set_string(book_object, "author", all_use_buffer);

    free(all_use_buffer);
    all_use_buffer = (char*)calloc(BUFLEN, sizeof(char));

    printf("genre=");
    fgets(all_use_buffer, BUFLEN, stdin);
    all_use_buffer[strlen(all_use_buffer) - 1] = '\0';
    json_object_set_string(book_object, "genre", all_use_buffer);

    free(all_use_buffer);
    all_use_buffer = (char*)calloc(BUFLEN, sizeof(char));

    printf("publisher=");
    fgets(all_use_buffer, BUFLEN, stdin);
    all_use_buffer[strlen(all_use_buffer) - 1] = '\0';

    char *page_count = (char*)calloc(BUFLEN, sizeof(char));
    printf("page_count=");
    fgets(page_count, BUFLEN, stdin);
    page_count[strlen(page_count) - 1] = '\0';

    if (session_cookie == NULL) {
        printf("ERROR - no user logged in\n");
        return;
    }
    
    // verificam daca inputul pt pagina e valid
    if (!verify_no(page_count)) {
        printf("ERROR - input for page_count bad\n");
        json_value_free(json_value);
        free(all_use_buffer);
        free(page_count);
        free(access_route);
        free(payload_type);
        close(socket_tcp);
        return;
    }

    json_object_set_number(book_object, "page_count", atoi(page_count));
    json_object_set_string(book_object, "publisher", all_use_buffer);

    char *data = json_serialize_to_string(json_value);

    // post request
    message = compute_post_request(HOST, access_route, payload_type, &data, strlen(data), session_cookie, token);
    free(data);

    // trimitem la server
    send_to_server(socket_tcp, message);
    char *response = receive_from_server(socket_tcp);
    free(message);

    if (contains_str(response, "error")) {
        printf("ERROR - couldn't add book\n");
    } else {
        printf("200 - OK - book added with SUCCESS\n");
    }

    json_value_free(json_value);
    free(response);
    free(all_use_buffer);
    free(page_count);
    free(access_route);
    free(payload_type);
    close(socket_tcp);
}

void logout_user(char **session_cookie, char **token) {

    if (*session_cookie == NULL) {
        printf("ERROR - no user logged in\n");
        return;
    }
    int socket_tcp = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    char *access_route = (char*)calloc(BUFLEN, sizeof(char));
    strcpy(access_route, "/api/v1/tema/auth/logout");

    message = compute_get_delete_request(HOST, access_route, NULL, *session_cookie, NULL, "GET");

    // trimitem serverului
    send_to_server(socket_tcp, message);
    char *response = receive_from_server(socket_tcp);
    free(message);
    free(access_route);
 
    // tratam erorile
    if (contains_str(response, "error")) {
        printf("ERROR - couldn't log out\n");
        free(response);
        close(socket_tcp);
        return;
    }
    
    printf("200 - OK - user logged out with SUCCESS\n");

    free(*session_cookie);
    *session_cookie = NULL;
    free(*token);
    *token = NULL;
    free(response);
    close(socket_tcp);
}

void exit_program(char *session_cookie, char *token, char *stdin_buffer, char *access_route, char *payload_type, char *username, char *password, char *all_use_buffer) {

    if (session_cookie != NULL)
        free(session_cookie);
    if (token != NULL)
        free(token);
    free(stdin_buffer);
    free(all_use_buffer);
    free(access_route);
    free(payload_type);
    free(username);
    free(password);
}


int main()
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    size_t len = LEN;
    size_t buffer_length = BUFLEN;
    
    char *stdin_buffer = calloc(LEN, sizeof(char));
    char *all_use_buffer = calloc(BUFLEN, sizeof(char));
    char *payload_type = calloc(BUFLEN, sizeof(char));
    char *access_route = calloc(BUFLEN, sizeof(char));
    char *username = calloc(BUFLEN, sizeof(char));
    char *password = calloc(BUFLEN, sizeof(char));
    char *cookie = NULL;
    char *token = NULL;

    printf("Enter a command:\n");
    while (1) {

        memset(stdin_buffer, 0, LEN);
        getline(&stdin_buffer, &len, stdin);
        stdin_buffer[strlen(stdin_buffer) - 1] = '\0';

        if (!strcmp(stdin_buffer, "register")) {
            register_user(buffer_length);
        }
        else if (!strcmp(stdin_buffer, "login")) {
            login_user(buffer_length, &cookie);
        }

        else if (!strcmp(stdin_buffer, "enter_library")) {
            enter_library(cookie, &token);
        }

        else if (!strcmp(stdin_buffer, "get_books")) {
            get_books(cookie, token);
        }

        else if (!strcmp(stdin_buffer, "get_book")) {
            get_book(buffer_length, cookie, token);
        }

        else if (!strcmp(stdin_buffer, "delete_book")) {
            delete_book(buffer_length, cookie, token);
        }

        else if (!strcmp(stdin_buffer, "add_book")) {
            add_book(buffer_length, cookie, token);
        }

        else if (!strcmp(stdin_buffer, "logout")) {
            logout_user(&cookie, &token);
        }

        else if (!strcmp(stdin_buffer, "exit")) {
            exit_program(cookie, token, stdin_buffer, access_route, payload_type, username, password, all_use_buffer);
            break;
        }
        else {
            printf("Wrong command.\n");
        }
    }

    return 0;
}