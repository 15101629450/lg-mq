
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adx_queue.h"
#include "adx_command_parse.h"

#define COMMAND_UNDONE		1
#define COMMAND_HEAD_OK     2
#define COMMAND_SUCCESS		3
#define COMMAND_ERROR		4

/************
 *3\r\n
 $2\r\n
 aa\r\n
 $2\r\n
 bb\r\n
 $2\r\n
 cc\r\n
 ************/

typedef struct {

    int len;
    char *buf;
    char *pos;

    int param_total;
    int param_count;

    int status;

    adx_list_t param_list;

} command_t_bak;


void command_display(command_t *c)
{
    adx_list_t *p = NULL;
    fprintf(stdout, "[CMD]");
    adx_list_for_each(p, &c->param_list) {
        adx_queue_t *node = (adx_queue_t *)p;
        fprintf(stdout, "[%s]", node->str.str);
    }

    fprintf(stdout, "\n");
}

int command_handle_parse(command_t *c)
{
    c->pos = NULL;
    c->param_total = 0;
    c->param_count = 0;
    c->status = COMMAND_UNDONE;
    adx_list_init(&c->param_list);

    char *buf = c->buf;
    if (c->len <= 5) return -1; // [*N\r\n$]最小头长度(报文未接收完成)
    if (*buf != '*') goto err; // 指令格式错误

    char *line = strchr(buf, '\r');
    if (!line || !line[1] || !line[2]) return -1; // 报文未接收完成
    if (line[1] != '\n' || line[2] != '$') goto err; // 指令格式错误

    c->param_total = atoi(buf + 1); // 跳过*
    if (c->param_total <= 0) goto err;

    c->pos = line + 2; // \r\n
    c->status = COMMAND_HEAD_OK;
    return 0;
err:
    c->status = COMMAND_ERROR;
    return -1;
}

int command_param_parse(command_t *c)
{
    char *buf = c->pos;
    if (!buf || !buf[0]) return -1;
    if (buf[0] != '$') goto err;

    char *line = strchr(buf + 1, '\r'); // 跳过$
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
    adx_str_t str = adx_queue_push_str(&c->param_list, line, len);
    if (adx_empty(str)) goto err;

    // param_total check
    c->param_count++;
    if (c->param_count >= c->param_total) {
        c->status = COMMAND_SUCCESS;
        return 0;
    }

    c->pos = line + len + 2;
    return command_param_parse(c);
err:
    c->status = COMMAND_ERROR;
    return -1;
}

int command_parse(command_t *c)
{
    if (c->status == COMMAND_UNDONE)
        command_handle_parse(c);

    if (c->status == COMMAND_HEAD_OK)
        command_param_parse(c);

    // if (c->status == COMMAND_ERROR) return -1;
    if (c->status == COMMAND_SUCCESS) command_display(c);
    return 0;
}

void command_free(command_t *c)
{
    c->pos = NULL;
    c->param_total = 0;
    c->param_count = 0;
    c->status = COMMAND_UNDONE;
    adx_queue_clear(&c->param_list);
}


