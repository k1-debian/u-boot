FROM ubuntu:18.04 AS build

RUN apt-get update && apt-get install -y build-essential gcc-mipsel-linux-gnu

WORKDIR /build
COPY . ./

RUN make -C /build O=/build/uboot-intermediate CROSS_COMPILE=mipsel-linux-gnu- crealityk1_uImage_msc0

RUN tar -czf uboot.tar.gz uboot-intermediate

FROM scratch AS export
COPY --from=build /build/uboot.tar.gz .
