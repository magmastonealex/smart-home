FROM alpine:latest

VOLUME /config

RUN apk add --no-cache ttf-dejavu

ADD einkserver /bin/einkserver

HEALTHCHECK --timeout=3s CMD (ps aux | grep einkserver) || exit 1

CMD ["/bin/einkserver", "/config/config.json"]
