# PHP 8.2 + Apache con pdo_pgsql y Composer.
FROM php:8.2-apache

RUN apt-get update && apt-get install -y --no-install-recommends \
        libpq-dev \
        git \
        unzip \
    && docker-php-ext-install pdo_pgsql pgsql \
    && a2enmod rewrite headers \
    && rm -rf /var/lib/apt/lists/*

# Composer (desde la imagen oficial)
COPY --from=composer:2 /usr/bin/composer /usr/bin/composer

WORKDIR /var/www/html

# Configuración Apache (front controller en public/)
COPY apache.conf /etc/apache2/sites-available/000-default.conf

COPY . .

RUN composer install --no-dev --optimize-autoloader --no-interaction

EXPOSE 80
