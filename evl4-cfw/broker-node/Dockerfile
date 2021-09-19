FROM node:14-alpine

VOLUME /config

ADD app /app

RUN cd app && npm install

CMD ["node", "/app/index.js", "/config/config.json"]
