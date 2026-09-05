ARG UID=1000
ARG GID=1000
ARG USERNAME=aurabot-dev

FROM ubuntu:24.04

ARG UID
ARG GID
ARG USERNAME

ENV DEBIAN_FRONTEND=noninteractive

ENV AURABOT_WORKSPACE=/workspace

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        wget \
        git \
        sudo \
        file \
        build-essential \
        cmake \
        ninja-build \
        pkg-config \
        python3 \
        python3-pip \
        python3-venv \
        openssh-client \
        qemu-system-arm \
        qemu-utils \
        qemu-efi-aarch64 \
        cloud-image-utils \
        gcc-aarch64-linux-gnu \
        binutils-aarch64-linux-gnu \
        libc6-dev-arm64-cross \
        gdb-multiarch && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*


RUN set -eux; \
    if getent group "${GID}" >/dev/null; then \
        GROUP_NAME="$(getent group "${GID}" | cut -d: -f1)"; \
    else \
        groupadd --gid "${GID}" "${USERNAME}"; \
        GROUP_NAME="${USERNAME}"; \
    fi; \
    if getent passwd "${UID}" >/dev/null; then \
        EXISTING_USER="$(getent passwd "${UID}" | cut -d: -f1)"; \
        usermod --gid "${GROUP_NAME}" "${EXISTING_USER}"; \
        if [ "${EXISTING_USER}" != "${USERNAME}" ]; then \
            usermod \
                --login "${USERNAME}" \
                --home "/home/${USERNAME}" \
                --move-home \
                "${EXISTING_USER}"; \
        fi; \
    else \
        useradd \
            --create-home \
            --uid "${UID}" \
            --gid "${GROUP_NAME}" \
            --shell /bin/bash \
            "${USERNAME}"; \
    fi; \
    echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" \
        > "/etc/sudoers.d/${USERNAME}"; \
    chmod 0440 "/etc/sudoers.d/${USERNAME}"


COPY docker/qemu/scripts/run-qemu.sh /usr/local/bin/run-qemu

RUN chmod +x /usr/local/bin/run-qemu


USER ${USERNAME}

WORKDIR /workspace

RUN echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc && \
    echo 'export PS1="[\u@\h \W]\\$ "' >> ~/.bashrc


EXPOSE 2222 5000 1234

CMD ["run-qemu"]