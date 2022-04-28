from alpine:3.15.0

RUN apk add --no-cache bash jq borgbackup openssh

ADD support/runme.bash /runme.bash

ENTRYPOINT "/runme.bash"

