#!/bin/bash

gcc socket_server.c -o socket_server  `mysql_config --cflags --libs`
./socket_server
