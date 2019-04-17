
CC	= gcc
INCLUDE	= -I$(SRCPATH) 
CFLAG	= -Wall -g
LIBS	= -ljemalloc
SRCPATH = ./src/
OBJPATH = ./obj/
APPNAME	= app_MQ_Server

OBJECTS += $(OBJPATH)adx_list.o
OBJECTS += $(OBJPATH)adx_rbtree.o
OBJECTS += $(OBJPATH)adx_alloc.o
OBJECTS += $(OBJPATH)adx_queue.o

OBJECTS += $(OBJPATH)adx_event.o
OBJECTS += $(OBJPATH)adx_network.o
OBJECTS += $(OBJPATH)adx_command_parse.o
OBJECTS += $(OBJPATH)main.o

all: clean $(OBJECTS)
	$(CC) $(INCLUDE) $(OBJECTS) -o $(APPNAME) $(LIBS) $(CFLAG)

$(OBJPATH)%.o:$(SRCPATH)%.c
	$(CC) $(INCLUDE) -c $< -o $@ $(CFLAG)

clean:
	@rm -f $(OBJECTS)
	@rm -f $(APPNAME)

