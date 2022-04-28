FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive
RUN apt update
RUN apt -y install sudo vim bash-completion wget gnupg2 lsb-core unzip net-tools supervisor curl 

# Install postgresql and patroni
RUN wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | apt-key add -
RUN echo "deb http://apt.postgresql.org/pub/repos/apt/ `lsb_release -cs`-pgdg main" >  /etc/apt/sources.list.d/pgdg.list
RUN apt update && apt -y install postgresql-13 postgresql-client-13 patroni python3-consul

COPY ./supervisord.conf /etc/supervisord.conf
COPY ./bootstrap-patroni.sh /usr/bin/bootstrap-patroni.sh

CMD ["/usr/bin/supervisord", "--nodaemon"]
