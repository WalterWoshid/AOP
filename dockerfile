FROM thecodingmachine/php:8.1-v4-fpm

USER root

# Install dependencies
RUN apt-get update \
    && apt install -y php8.1-dev

# Copy files so we can use the scripts
COPY ./ /opt/aop

WORKDIR /opt/aop

# Build and install the library
RUN ./bin/build-install.sh