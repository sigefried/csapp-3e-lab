#include <stdio.h>
#include "csapp.h"
#include <string.h>

//#define DEBUG

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n\r\n";
static const char *default_http_version = "HTTP/1.0";

//funtions definitaion
void serve(int fd);
void read_requesthdrs(rio_t *rp);
void client_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int parse_uri(char *orig_uri, char *host, int *port, char *path);

// thread function
void *thread_serv(void *args);


int main(int argc, char **argv) {
	int listenfd, connfd;
	char hostname[MAXLINE], port[MAXLINE];
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	pthread_t tid;

	/* Check command line args */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	/* Ignore SIGPIPE signal */
	Signal(SIGPIPE, SIG_IGN);

	listenfd = Open_listenfd(argv[1]);
	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr,
				&clientlen);
		Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
				0);
		printf("[LOG]:Accepted connection from (%s, %s)\n", hostname, port);
		Pthread_create(&tid, NULL, thread_serv, (void *)(&connfd));
	}
}

void *thread_serv(void *args) {
	int connfd = *((int *) args);
	Pthread_detach(pthread_self());
	printf("[LOG]:New thread created for connfd:%d ...\n", connfd);
	serve(connfd);
	Close(connfd);
	printf("[LOG]:Thread finished...\n");
	return NULL;
}

void serve(int fd) {
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char host[MAXLINE], path[MAXLINE];
	char payload[MAX_OBJECT_SIZE], buf_payload[MAXLINE];
	int n;
	int sum_byte = 0;
	int port;
	char port_char[6];
	char new_request[MAXLINE];
	rio_t client_rio, server_rio;
	int server_fd;
	/* Read request line and headers */
	Rio_readinitb(&client_rio, fd);
	if (!Rio_readlineb(&client_rio, buf, MAXLINE))
		return;
	sscanf(buf, "%s %s %s", method, uri, version);
	if (strcasecmp(method, "GET")) {
		client_error(fd, method, "501", "Not Implemented",
				"Proxy does not implement this method");
		return;
	}
	//parse header
	read_requesthdrs(&client_rio);

	memset(path, 0, MAXLINE);
	if (parse_uri(uri, host, &port, path) < 0 ) {
		client_error(fd, uri, "404", "Not found",
				"The proxy couldn't parse the request");
		return;
	}

#ifdef DEBUG
	{
		printf("------- AFTER URI PARSE ---------\n");
		printf("host: %s, port: %d, path: %s\n", host, port, path);
		printf("---------------------------------\n");
	}
#endif

	//build_request
	memset(new_request, 0, MAXLINE);
	sprintf(new_request, "%s %s %s\r\n", method, path, default_http_version);

	//concat new request
	strcat(new_request, user_agent_hdr);
	strcat(new_request, accept_hdr);
	strcat(new_request, accept_encoding_hdr);
	strcat(new_request, connection_hdr);
	strcat(new_request, proxy_connection_hdr);

#ifdef DEBUG
	{
		printf("------- AFTER BUILD NEW REQUEST ---------\n");
		printf("new_request: %s\n", new_request);
		printf("---------------------------------\n");
	}
#endif

	memset(payload, 0, sizeof(payload));
	memset(port_char, 0 , sizeof(port_char));
	sprintf(port_char, "%d", port);
	server_fd = Open_clientfd(host, port_char);

	// write request
	Rio_writen(server_fd, new_request, MAXLINE);
	Rio_readinitb(&server_rio, server_fd);
	while ((n = Rio_readlineb(&server_rio, buf_payload, MAXLINE)) != 0 ) {
		sum_byte += n;
		if (sum_byte <= MAX_OBJECT_SIZE) {
			strcat(payload, buf_payload);
		}
		Rio_writen(fd, buf_payload, n);
	}
	printf("[LOG]:Forwar %d bytes content\n", sum_byte);


}

int parse_uri(char *orig_uri, char *host, int *port, char *path) {
	char *ptr, *end_of_host;
	char *pos_of_comm;
	*port = 80;

	ptr = strstr(orig_uri, "http://");
	if (ptr == NULL) {
		strcpy(host, orig_uri);
		return 0;
	} else {
		ptr +=7;
		//parse host
		end_of_host = strchr(ptr, '/');
		if (end_of_host != NULL) {
			*end_of_host ='\0';
			strcpy(host, ptr);
			//parse path
			*end_of_host = '/';
			strcpy(path, end_of_host);
		} else {
			strcpy(host, ptr);
			strcpy(path,"/");
		}

		pos_of_comm = strchr(host, ':');
		if (pos_of_comm != NULL) {
			*pos_of_comm = '\0';
			*port = atoi(pos_of_comm + 1);
		}
		return 0;
	}

	return -1;
}

/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t *rp) {
	char buf[MAXLINE];

	Rio_readlineb(rp, buf, MAXLINE);
	printf("========original req header start======\n");
	printf("%s", buf);
	while (strcmp(buf, "\r\n")) { // line:netp:readhdrs:checkterm
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
	}
	printf("========original req header end========\n");
	return;
}

/*
 * Build a simple website for cannot connect to server errors
 */
void client_error(int fd, char *cause, char *errnum,
		char *shortmsg, char *longmsg) {
	char buf[MAXLINE], body[MAXLINE];

	/* Build the HTTP response body */
	sprintf(body, "<html><title>Request Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The proxy</em>\r\n", body);

	/* Print the HTTP response */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));
	Rio_writen(fd, body, strlen(body));
}
