from alpine:3.15.0

RUN apk add --no-cache unbound && chown 100:101 /etc/unbound

CMD [ "unbound", "-d", "-c", "/etc/unbound/unbound.conf" ]
