FROM alpine:latest

EXPOSE 8473
VOLUME /config

ADD mqtt_cacher /bin/mqtt_cacher

RUN apk --update --no-cache add curl

HEALTHCHECK --timeout=3s CMD curl -s -f http://127.0.0.1:8473/getInfo || exit 1

CMD ["/bin/mqtt_cacher"]
