#!/bin/bash
set -e

echo "========================================"
echo "  to_gif 依赖安装脚本"
echo "========================================"

# 检测发行版
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
else
    echo "错误：无法检测发行版"
    exit 1
fi

echo "检测到发行版: $DISTRO"

install_debian() {
    echo "--> 更新软件包列表..."
    sudo apt-get update

    echo "--> 安装基础构建工具..."
    sudo apt-get install -y \
        build-essential \
        cmake \
        pkg-config

    echo "--> 安装 FFmpeg 开发库..."
    sudo apt-get install -y \
        libavcodec-dev \
        libavformat-dev \
        libswscale-dev \
        libavutil-dev

    echo "--> 安装 giflib 开发库..."
    sudo apt-get install -y \
        libgif-dev

    echo "--> 安装 libimagequant..."
    # 尝试从仓库安装
    if apt-cache show libimagequant-dev >/dev/null 2>&1; then
        sudo apt-get install -y libimagequant-dev
    else
        echo "    仓库中没有 libimagequant-dev，将从源码构建..."
        install_imagequant_from_source
    fi
}

install_imagequant_from_source() {
    local TMPDIR=$(mktemp -d)
    cd "$TMPDIR"

    echo "--> 下载 libimagequant 源码..."
    git clone --depth 1 https://github.com/ImageOptim/libimagequant.git
    cd libimagequant

    echo "--> 编译并安装 libimagequant..."
    ./configure --prefix=/usr/local
    make -j$(nproc)
    sudo make install

    # 更新 pkg-config 路径
    sudo ldconfig

    cd /
    rm -rf "$TMPDIR"
    echo "--> libimagequant 源码安装完成"
}

install_fedora() {
    echo "--> 安装依赖..."
    sudo dnf install -y \
        gcc-c++ \
        cmake \
        pkgconfig \
        libavcodec-free-devel \
        libavformat-free-devel \
        libswscale-free-devel \
        libavutil-free-devel \
        libimagequant-devel \
        giflib-devel
}

install_arch() {
    echo "--> 安装依赖..."
    sudo pacman -S --noconfirm \
        base-devel \
        cmake \
        pkgconf \
        ffmpeg \
        libimagequant \
        giflib
}

# 根据发行版分发
case "$DISTRO" in
    ubuntu|debian|pop|linuxmint)
        install_debian
        ;;
    fedora|rhel|centos)
        install_fedora
        ;;
    arch|manjaro)
        install_arch
        ;;
    *)
        echo "错误：不支持的发行版: $DISTRO"
        echo "支持的发行版: Ubuntu, Debian, Fedora, Arch"
        exit 1
        ;;
esac

echo ""
echo "========================================"
echo "  所有依赖安装完成！"
echo "========================================"
echo ""
echo "现在可以构建项目了:"
echo "  mkdir -p build && cd build"
echo "  cmake .."
echo "  make -j\$(nproc)"
