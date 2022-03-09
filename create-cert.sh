#!/bin/env bash

mkdir -p data
openssl req -x509 -newkey rsa:4096 -keyout data/key.pem -out data/cert.pem -sha256 -days 3650