BEARER_TOKEN=$(cat bearertoken.txt)
PATH="$PATH":/home/alex/kindlejb/dev/armv5-eabi--glibc--stable-2018.02-2/bin/
arm-linux-gcc formtest.c main.c -D"BEARER_TOKEN=$BEARER_TOKEN" -g -I/home/alex/kindlejb/dev/include/ -L/home/alex/kindlejb/dev/lib/ -lX11 -lxcb -lXau -lXdmcp -lXpm -lforms -lpaho-mqtt3c -lcares -lz -lcrypto -lssl -lcurl -o formtest

