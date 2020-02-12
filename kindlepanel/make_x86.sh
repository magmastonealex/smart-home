BEARER_TOKEN=$(cat bearertoken.txt)
gcc formtest.c main.c -D"BEARER_TOKEN=$BEARER_TOKEN" -lX11 -lxcb -lXau -lXdmcp -lXpm -lforms -lpaho-mqtt3a -lpaho-mqtt3c -lcares -lz -lcrypto -lssl -lcurl -o formtest
