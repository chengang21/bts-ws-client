# bts-ws-client

build:

gcc -g -O2 -o bts-wscli bts-wscli.c sha1.c base64.c

gcc -g -O2 -o bts-wscli-ssl bts-wscli-ssl.c sha1.c base64.c -lssl


./bts-wscli-ssl wss://bit.btsabc.org/ws
