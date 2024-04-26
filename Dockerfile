FROM ubuntu:22.04
RUN apt-get update && apt install nginx php-fpm mariadb-server phpmyadmin -y
EXPOSE 3306/tcp
