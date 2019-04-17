
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define RECV_BUF_SIZE 4096

typedef struct {

    int type;

    char *host;
    int host_size;
    int port;

    char *send_buf;
    int send_size;

    char recv_buf[RECV_BUF_SIZE];
    int recv_size;

} mq_shell_t;

int mq_shell_code(int sockfd, int code)
{
    close(sockfd);
    return code;
}

int mq_shell(mq_shell_t *p)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd <= 0) return mq_shell_code(sockfd, -1);

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_aton(p->host, &addr.sin_addr);
    addr.sin_port = htons(p->port);
    int ret = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret) return mq_shell_code(sockfd, -1);

    char buffer[4096];
    int size = sprintf(buffer, "%d%04d", p->type, p->send_size);
    memcpy(&buffer[size], p->send_buf, p->send_size);

    ret = write(sockfd, buffer, size + p->send_size);
    if (ret <= 0) return mq_shell_code(sockfd, -1);

    p->recv_size = read(sockfd, p->recv_buf, RECV_BUF_SIZE);
    if (ret <= 0) return mq_shell_code(sockfd, -1);

    p->recv_buf[p->recv_size] = 0;
    if (p->recv_size == 3 && strcmp(p->recv_buf, "err") == 0)
	return mq_shell_code(sockfd, -1);

    return mq_shell_code(sockfd, 0);
}

PyObject *push(PyObject *self, PyObject *args)
{
    mq_shell_t shell;
    if (!PyArg_ParseTuple(args, "s#is#", &shell.host, &shell.host_size, &shell.port, &shell.send_buf, &shell.send_size))
	Py_RETURN_NONE;

    shell.type = 1;
    if (mq_shell(&shell)) Py_RETURN_NONE;
    return (PyObject *)Py_BuildValue("s#", shell.recv_buf, shell.recv_size);
}

PyObject *push2(PyObject *self, PyObject *args)
{
    mq_shell_t shell;
    if (!PyArg_ParseTuple(args, "s#is#", &shell.host, &shell.host_size, &shell.port, &shell.send_buf, &shell.send_size))
	Py_RETURN_NONE;

    shell.type = 2;
    if (mq_shell(&shell)) Py_RETURN_NONE;
    return (PyObject *)Py_BuildValue("s#", shell.recv_buf, shell.recv_size);
}

PyObject *pop(PyObject *self, PyObject *args)
{
    mq_shell_t shell;
    if (!PyArg_ParseTuple(args, "s#i", &shell.host, &shell.host_size, &shell.port))
	Py_RETURN_NONE;

    shell.type = 3;
    shell.send_size = 0;
    if (mq_shell(&shell)) Py_RETURN_NONE;
    return (PyObject *)Py_BuildValue("s#", shell.recv_buf, shell.recv_size);
}

PyObject *total(PyObject *self, PyObject *args)
{
    mq_shell_t shell;
    if (!PyArg_ParseTuple(args, "s#i", &shell.host, &shell.host_size, &shell.port))
	Py_RETURN_NONE;

    shell.type = 4;
    shell.send_size = 0;
    if (mq_shell(&shell)) Py_RETURN_NONE;
    return (PyObject *)Py_BuildValue("s#", shell.recv_buf, shell.recv_size);
}

PyObject *count(PyObject *self, PyObject *args)
{
    mq_shell_t shell;
    if (!PyArg_ParseTuple(args, "s#i", &shell.host, &shell.host_size, &shell.port))
	Py_RETURN_NONE;

    shell.type = 5;
    shell.send_size = 0;
    if (mq_shell(&shell)) Py_RETURN_NONE;
    return (PyObject *)Py_BuildValue("s#", shell.recv_buf, shell.recv_size);
}

PyObject *event(PyObject *self, PyObject *args)
{
    mq_shell_t shell;
    if (!PyArg_ParseTuple(args, "s#i", &shell.host, &shell.host_size, &shell.port))
	Py_RETURN_NONE;

    shell.type = 6;
    shell.send_size = 0;
    if (mq_shell(&shell)) Py_RETURN_NONE;
    return (PyObject *)Py_BuildValue("s#", shell.recv_buf, shell.recv_size);
}

PyMethodDef lg_mq_methods[] = {
    {"push",	push,	METH_VARARGS,	NULL},
    {"push2",	push2,	METH_VARARGS,	NULL},
    {"pop",	pop,	METH_VARARGS,	NULL},
    {"total",	total,	METH_VARARGS,	NULL},
    {"count",	count,	METH_VARARGS,	NULL},
    {"event",	event,	METH_VARARGS,	NULL},
    {NULL,	NULL,	0,		NULL}
};

PyMODINIT_FUNC initlg_mq() 
{
    Py_InitModule("lg_mq", lg_mq_methods);
}


