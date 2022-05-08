# clfilters

clfilters.auf は、OpenCL経由でGPUを使用し各種フィルタ処理を一括で行います。

フィルタ処理をGPU上で連続して行うことで、フィルタをそれぞれ実行するのと比べてCPU - GPU間のデータ転送を削減でき、高速化が期待できます。

## 想定動作環境

Windows 8.1/10/11 (x86/x64)  
Aviutl 1.00 以降  
Intel / NVIDIA / AMD のGPUドライバのインストールされた環境  

## clfilters 使用にあたっての注意事項

無保証です。自己責任で使用してください。clfiltersを使用したことによる、いかなる損害・トラブルについても責任を負いません。  

今後の更新でプロファイルの互換性がなくなるかもしれません。  
※非常にありえます。

## 使用可能なフィルタと適用順

フィルタについては下記の順番で適用されます。

- 色空間変換
- nnedi
- ノイズ除去(knn)
- ノイズ除去(pmd)
- ノイズ除去(smooth)
- リサイズ
- unsharp
- エッジレベル調整
- warpsharp
- バンディング低減
- 色調補正

## 課題

clfilters には下記の課題があります。

- NVIDIAのGPU等、[cl_khr_image2d_from_buffer](https://www.khronos.org/registry/OpenCL/sdk/3.0/docs/man/html/cl_khr_image2d_from_buffer.html) というKHR拡張がサポートされない環境で無駄にメモリコピーが多発する。  
  OpenCL 2.0でこの拡張は標準になったので、いろいろなフィルタを cl_khr_image2d_from_buffer ありきで実装してきたのですが、
  OpenCL 3.0で標準から外れてしまい、NVIDIA GPUでは対応していないようです(Intel/AMDは対応)。
  CUDAのテクスチャバインドとほぼ同じなのになんでサポートしないの…。悲しみ。

- 時間方向に参照するフィルタに未対応。  
  vpp-convolution3d など。実装がややこしいので見送り中です。

## コンパイル環境

VC++ 2022 Community



