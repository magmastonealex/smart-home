from alpine:3.15.0

RUN apk add --no-cache bash bird bind-tools

ADD support/runme.bash /runme.bash
ADD support/hc.sh /hc.sh
ADD support/bird.conf.tmpl /bird.conf.tmpl 

HEALTHCHECK CMD /hc.sh

CMD ["/runme.bash"]

