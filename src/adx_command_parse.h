
#ifndef __ADX_COMMAND_PARSE_H__
#define __ADX_COMMAND_PARSE_H__

#define COMMAND_UNDONE		1
#define COMMAND_HEAD_OK     2
#define COMMAND_SUCCESS		3
#define COMMAND_ERROR		4

#ifdef __cplusplus
extern "C" { 
#endif

    typedef struct {

        int len;
        char *buf;
        char *pos;

        int param_total;
        int param_count;
        adx_list_t param_list;

        int status;

    } command_t;

    int command_parse(command_t *c);
    void command_free(command_t *c);

#ifdef __cplusplus
}
#endif

#endif



