# syntax=docker/dockerfile:1

# DOCKER_BUILDKIT=1 docker build -o=./dist .

FROM ghcr.io/toltec-dev/qt AS setup
WORKDIR /w
COPY dist.sh .
COPY rm2fb.pro .
COPY scripts/ ./scripts
COPY src/ ./src

FROM setup AS build
RUN ./dist.sh

FROM setup AS build-static-client
RUN echo 'CONFIG += staticlib' >>src/client/client.pro
RUN qmake
RUN make

FROM scratch AS static-client
COPY --from=build-static-client /w/src/client/librm2fb_client.a /

FROM scratch AS dynamic-libraries
COPY --from=build /w/dist /
