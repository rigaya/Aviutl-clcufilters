## clcufiltersのビルド方法

### 0. ビルドに必要なもの
ビルドには、下記のものが必要です。

- Visual Studio 2019/2022
- CUDA 11.0 - 12.3
- [OpenCL Headers](https://github.com/KhronosGroup/OpenCL-Headers)

### 1. 環境準備

Visual Studioをインストール後、CUDAをインストールします。

その後、ビルドに必要なOpenCLのヘッダをダウンロードし、環境変数 ```OPENCL_HEADERS``` にその場所を設定します。

```Batchfile
git clone https://github.com/KhronosGroup/OpenCL-Headers.git <path-to-clone>
setx OPENCL_HEADERS <path-to-clone>
```

### 2. ソースのダウンロード

submoduleのダウンロードも必要なため、gitコマンドで```--recursive```付きでダウンロードします。

```Batchfile
git clone https://github.com/rigaya/Aviutl-clcufilters --recursive
```

### 3. clcufilters.auf / clfilters.exe / cufilters.exe のビルド

clcufilters.slnを開き、ビルドします。

ビルドしたいものに合わせて、構成を選択してください。

|                      |Debug用構成   |Release用構成|
|:---------------------|:------|:--------|
| clcufilters.auf      | Debug (Win32) | Release (Win32) |
| clfilters.exe        | Debug (x64)   | Release (x64)   |
| cufilters.exe        | DebugCU (x64) | ReleaseCU (x64) |