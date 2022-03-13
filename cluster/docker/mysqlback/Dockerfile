from alpine:3.15.0

RUN apk add --no-cache bash jq mariadb-client mariadb-connector-c

ADD support/runme.bash /runme.bash

ENTRYPOINT "/runme.bash"

