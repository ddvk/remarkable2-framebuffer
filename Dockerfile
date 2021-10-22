# syntax=docker/dockerfile:1

# DOCKER_BUILDKIT=1 docker build -o=./dist .

FROM ghcr.io/toltec-dev/qt AS build
WORKDIR /w
COPY . .
RUN ./dist.sh

FROM scratch
COPY --from=build /w/dist /
