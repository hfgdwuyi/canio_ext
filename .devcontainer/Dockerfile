FROM mcr.microsoft.com/vscode/devcontainers/base:trixie

RUN apt-get update \
	&& apt-get install -y --no-install-recommends \
		ca-certificates \
		cmake \
		libjaylink0 \
		ninja-build \
		openocd \
		srecord \
		udev \
		usbutils \
		wget \
		xz-utils \
	&& rm -rf /var/lib/apt/lists/*
#RUN apt install net-tools -y
#RUN apt install python3 -y
#RUN apt install python3-pip -y
#RUN pip install canopen --break-system-packages
#RUN pip install gcovr --break-system-packages
#RUN pip install vpython --break-system-packages
#RUN pip install canalystii --break-system-packages
#RUN pip install "setuptools<82" --break-system-packages

# Download ARM GNU toolchain
RUN wget https://developer.arm.com/-/media/Files/downloads/gnu/14.3.rel1/binrel/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz
# Unpack tarball
RUN tar xf arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz -C /opt \
	&& rm -f arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz

ENV PATH="/opt/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin:${PATH}"

