from alpine:3.15.0

RUN apk add --no-cache bash jq postgresql

ADD support/runme.bash /runme.bash

ENTRYPOINT "/runme.bash"

