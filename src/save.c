
#include "ae.h"
#include "anet.h"
#include "adx_queue.h"

#define CONFIG_BINDADDR_MAX 16
#define MAX_ACCEPTS_PER_CALL 1000
#define NET_IP_STR_LEN 46
#define CONFIG_DEFAULT_TCP_KEEPALIVE 300
#define PROTO_IOBUF_LEN         (1024*1024)
#define PROTO_REQ_INLINE 1
#define PROTO_REQ_MULTIBULK 2

#define COMMAND_UNDONE		1
#define COMMAND_SUCCESS		2
#define COMMAND_ERROR		3
#define COMMAND_RECV_FULL	4
#define COMMAND_SEND_FULL	5

typedef struct {

	pid_t pid;
	aeEventLoop *el;

	int tcp_backlog;
	int tcp_keepalive;
	int port;

	int sock_listen;
	char err[ANET_ERR_LEN];

} redisServer;

typedef struct {

	int fd;
	int flags;

	int len;
	char *buf;
	char *pos;

	int argc;
	int argc_count;
	int status;

	adx_list_t cmd;

	char *send_buf;
	int send_len;

} redisClient;

redisServer server;

char ch_check(char ch)
{
	if (ch >= 33 && ch <= 126) return ch;
	return 0;
}

void buf_display(char *buf, int size)
{
	int i;
	buf[size] = 0;
	for (i = 0; i < size; i++)
		fprintf(stdout, "[%03d][%03d][%c]\n", i, buf[i], ch_check(buf[i]));
	fprintf(stdout, "===============================\n");
	fflush(stdout);
}

void freeClient(redisClient *c)
{
	if (!c) return;

	aeDeleteFileEvent(server.el, c->fd, AE_READABLE);
	aeDeleteFileEvent(server.el, c->fd, AE_WRITABLE);
	close(c->fd);
	c->fd = -1;

	if (c->buf) je_free(c->buf);
	je_free(c);
}

int command_handle_parse(redisClient *c)
{
	char *buf = c->buf;

	c->argc = 0;
	c->argc_count = 0;
	c->status = COMMAND_UNDONE;
	c->pos = NULL;

	if (!buf[0]) return -1; // 报文未接收完成(等待下次判断)
	if (buf[0] != '*') goto err; // 指令格式错误

	char *line = strchr(buf, '\r');
	if (!line || !line[1] || !line[2]) return -1; // 报文未接收完成
	if (line[1] != '\n' || line[2] != '$') goto err; // 指令格式错误

	c->argc = atoi(buf + 1);
	if (!c->argc || c->argc >= 100) goto err;

	c->pos = line + 2; // \r\n
	return 0;
err:
	c->status = COMMAND_ERROR;
	return -1;
}

int command_argv_parse(redisClient *c)
{
	char *buf = c->pos;
	c->status = COMMAND_UNDONE;

	if (!buf || !buf[0]) return -1;
	if (buf[0] != '$') goto err;

	char *line = strchr(buf + 1, '\r'); // buf[0] = $
	if (!line || !line[1]) return -1;
	if (line[1] != '\n') goto err;
	line++; // \r
	line++; // \n

	int len = atoi(buf + 1);
	if (len <= 0) goto err;

	// 已收到的数据剩余长度
	int left_len = c->len - (line - c->buf);

	// 检测len长度与剩余的长度(数据是否已收全)
	if (left_len < (len + 2)) return -1; // 缓冲区数据未收全 len + 2 = \r\n

	if (line[len] != '\r' || line[len + 1] != '\n') goto err;

	// push cmd
	adx_str_t str = adx_queue_push_str(&c->cmd, line, len);
	if (adx_empty(str)) goto err;

	// argc check
	c->argc_count++;
	if (c->argc_count >= c->argc) c->status = COMMAND_SUCCESS;

	c->pos = line + len + 2;
	return 0;
err:
	c->status = COMMAND_ERROR;
	return -1;
}

void command_display(redisClient *c)
{
	adx_list_t *p = NULL;
	fprintf(stdout, "[CMD]");
	adx_list_for_each(p, &c->cmd) {
		adx_queue_t *node = (adx_queue_t *)p;
		fprintf(stdout, "[%s]", node->str.str);
	}

	fprintf(stdout, "\n");
}

void command_parse(redisClient *c)
{
	if (command_handle_parse(c) != 0)
		return;
	int i;	
	for (i = 0; i < c->argc; i++) {
		if (command_argv_parse(c) != 0) {
			adx_queue_clear(&c->cmd);
			return;
		}
	}
}

void sendReplyToClient(aeEventLoop *el, int fd, void *p, int mask);
int command_success(redisClient *c)
{
	// command_display(c);

	int left_len = c->len - (c->pos - c->buf);
	if (left_len > 0) {
		memcpy(c->buf, c->pos, left_len);
		c->len = left_len;

	} else {
		c->len = 0;
	}

	static int count = 1;
	if (count++ >= 10000) {
		fprintf(stdout, "1W\n");
		count = 1;
	}

#if 0
	// TODO : call dict
	char buf[128];
	static int index = 0;
	// int size = sprintf(buf, "$10\r\n%010d\r\n", ++index);
	int size = sprintf(buf, "%010d", ++index);
	// TODO: call end
#endif
	adx_queue_pop(&c->cmd);
	adx_str_t str = adx_queue_pop(&c->cmd);

	char *buf = str.str;
	int size = str.len;
	
	adx_queue_clear(&c->cmd);
	c->status = COMMAND_UNDONE;
	
	if (PROTO_IOBUF_LEN - c->send_len <= 2) {
		c->status = COMMAND_SEND_FULL; // 发送缓冲区已满
		return -1;
	}

	// 放入发送缓冲区
	memcpy(c->send_buf + c->send_len, buf, size);
	c->send_len += size;

	aeCreateFileEvent(server.el, c->fd, AE_WRITABLE, sendReplyToClient, c);
	return 0;
}

int processBuffer(redisClient *c)
{
	// buf_display(c->buf, c->len);

	if (c->status == COMMAND_UNDONE) {
		command_parse(c);
	}

	if (c->status == COMMAND_SUCCESS) {
		int errcode = command_success(c);
		if (errcode) return errcode;
		return processBuffer(c);
	}

	if (c->status == COMMAND_ERROR) {
		fprintf(stdout, "COMMAND_ERROR\n");
		freeClient(c);
		return -1;
	}

	return 0;
}

void readQueryFromClient(aeEventLoop *el, int fd, void *p, int mask)
{
	redisClient *c = (redisClient *)p;

	int readlen = PROTO_IOBUF_LEN;
	if (readlen - c->len <= 2) {
		freeClient(c); // 缓冲区已满
		return;
	}

	int nread = read(fd, c->buf + c->len, readlen - c->len);
	if (nread == -1) {
		if (errno == EAGAIN) return;
		freeClient(c);
		return;

	} else if (nread == 0) {
		freeClient(c);
		return;
	}

	c->len += nread;
	c->buf[c->len] = 0;
	processBuffer(c);
}

void ReplyToClient(redisClient *c)
{
	int writelen = 0;
	while(c->send_len > 0) {
		writelen = write(c->fd, c->send_buf + writelen, c->send_len);
		if (writelen == -1) {
			if (errno == EAGAIN) return;
			freeClient(c);
			return;

		} else if (writelen == 0) {
			freeClient(c);
			return;
		}

		c->send_len = c->send_len - writelen;
	}

	if (c->send_len > 0) memcpy(c->send_buf, c->send_buf + writelen, c->send_len);
}

void sendReplyToClient(aeEventLoop *el, int fd, void *p, int mask)
{
	redisClient *c = (redisClient *)p;
	if (c->status == COMMAND_SEND_FULL) {
		fprintf(stdout, "COMMAND_SEND_FULL\n");
		freeClient(c);
		return;
	}

	int len = c->send_len;
	ReplyToClient(c);
	fprintf(stdout, "==>[%d][%d][%d]\n", PROTO_IOBUF_LEN, len, c->send_len);

	aeDeleteFileEvent(server.el, c->fd, AE_WRITABLE);
}

redisClient *redisClientCreate(int fd) 
{
	redisClient *c = zmalloc(sizeof(redisClient));
	if (!c) return NULL;

	c->fd = fd;
	c->flags = 0;

	c->len = 0;
	c->buf = zmalloc(PROTO_IOBUF_LEN); // 接收缓冲区 
	c->pos = NULL;

	c->argc = 0;
	c->argc_count = 0;
	c->status = COMMAND_UNDONE;

	adx_list_init(&c->cmd);

	c->send_buf = zmalloc(PROTO_IOBUF_LEN); // 发送缓冲区
	c->send_len = 0;

	if (!c->buf || !c->send_buf) {
		freeClient(c);
		return NULL;
	}

	anetNonBlock(NULL, fd);
	anetEnableTcpNoDelay(NULL, fd);
	anetKeepAlive(NULL, fd, server.tcp_keepalive);
	aeCreateFileEvent(server.el, fd, AE_READABLE, readQueryFromClient, c);
	// aeCreateFileEvent(server.el, fd, AE_WRITABLE, sendReplyToClient, c);

	return c;
}

static void acceptCommonHandler(int fd, int flags, char *ip) 
{

	redisClient *c;
	if ((c = redisClientCreate(fd)) == NULL) {
		close(fd);
		return;
	}

	c->flags |= flags;
}

void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask)
{
	char client_ip[NET_IP_STR_LEN];
	int client_port;

	int cfd = anetTcpAccept(server.err, fd, client_ip, NET_IP_STR_LEN, &client_port);
	if (cfd == ANET_ERR) {
		// if (errno != EWOULDBLOCK) fprintf(stderr, "client connection: %s\n", server.err);
		return;
	}

	acceptCommonHandler(cfd, 0, client_ip);
}

int initServer(void)
{
	server.pid = getpid();
	server.el = aeCreateEventLoop(1024);

	server.tcp_backlog = SOMAXCONN;
	server.tcp_keepalive = CONFIG_DEFAULT_TCP_KEEPALIVE;
	server.port = server.port ? server.port : 6379;

	server.sock_listen = anetTcpServer(server.err, server.port, NULL, server.tcp_backlog);
	if (server.sock_listen != ANET_ERR) {
		anetNonBlock(NULL, server.sock_listen);
	} else if (errno == EAFNOSUPPORT) {
		return -1;
	}

	return aeCreateFileEvent(server.el, server.sock_listen, AE_READABLE, acceptTcpHandler, NULL);
}

int main()
{
	initServer();
	aeMain(server.el);
	return 0;
}


