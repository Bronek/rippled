ARG DIST_TAG
FROM ubuntu:$DIST_TAG

WORKDIR /root

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update ;\
    apt-get install -y --install-recommends \
    vim wget curl git grep sed gdb zsh cmake ninja-build \
    clang-15 clang-tools-15 clang-tidy-15 clangd-15 llvm-15 \
    clang-format-11 \
    python3 python3-pip python3-venv ;\
    apt-get clean

ENV PATH=${PATH}:/usr/lib/llvm-15/bin:/usr/lib/llvm-11/bin

RUN wget -O /etc/zsh/zshrc https://git.grml.org/f/grml-etc-core/etc/zsh/zshrc ;\
    wget -O  /etc/zsh/newuser.zshrc.recommended  https://git.grml.org/f/grml-etc-core/etc/skel/.zshrc

ARG USER_NAME
ARG USER_UID
ARG USER_GID
RUN groupadd --gid ${USER_GID} ${USER_NAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} -m ${USER_NAME} -s /bin/zsh -d /home/${USER_NAME}

ARG WORKSPACE
RUN mkdir -p ${WORKSPACE} ;\
    chown ${USER_NAME}:${USER_NAME} ${WORKSPACE}

USER ${USER_NAME}
ENV HOME=/home/${USER_NAME}
WORKDIR ${HOME}

ENV VIRTUAL_ENV=${HOME}/venv
RUN python3 -m venv ${VIRTUAL_ENV}
ENV PATH=${VIRTUAL_ENV}/bin:${PATH}
ENV EDITOR=vim
ENV VISUAL=vim

RUN pip install 'conan<2' ;\
    conan profile new default --detect ;\
    conan profile update settings.compiler.cppstd=20 default ;\
    conan profile update settings.compiler.libcxx=libstdc++11 default ;\
    conan profile update 'conf.tools.build:compiler_executables={"c": "/usr/lib/llvm-15/bin/clang", "cpp": "/usr/lib/llvm-15/bin/clang++"}' default

RUN echo "mkdir .build" > TODO.txt ;\
    echo "ln -s .build/compile_commands.json compile_commands.json" >> TODO.txt ;\
    echo "cd .build" >> TODO.txt ;\
    echo "conan install .. --output-folder . --build missing --settings build_type=Debug" >> TODO.txt ;\
    echo "conan install .. --output-folder . --build missing --settings build_type=Release" >> TODO.txt ;\
    echo "cmake -DCMAKE_GENERATOR='Ninja Multi-Config' -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_TOOLCHAIN_FILE:FILEPATH=build/generators/conan_toolchain.cmake .." >> TODO.txt ;\
    echo "cmake --build . --config Release" >> TODO.txt ;\
    echo "cmake --build . --config Debug" >> TODO.txt

RUN cp /etc/zsh/newuser.zshrc.recommended .zshrc ;\
    touch .zshrc.local ;\
    ln -s .profile .zprofile
