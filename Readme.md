# clfilters

clfilters.auf �́AAviutl�p��OpenCL�ɂ��GPU�t�B���^�ł��B

�t�B���^������GPU��ŘA�����čs�����ƂŁA�t�B���^�����ꂼ����s����̂Ɣ�ׂ�CPU - GPU�Ԃ̃f�[�^�]�����팸�ł��A�����������҂ł��܂��B

## �z�蓮���

Windows 8.1/10/11 (x86/x64)  
Aviutl 1.00 �ȍ~  
Intel / NVIDIA / AMD ��GPU�h���C�o�̃C���X�g�[�����ꂽ��  

## clfilters �g�p�ɂ������Ă̒��ӎ���

���ۏ؂ł��B���ȐӔC�Ŏg�p���Ă��������Bclfilters���g�p�������Ƃɂ��A�����Ȃ鑹�Q�E�g���u���ɂ��Ă��ӔC�𕉂��܂���B  

����̍X�V�Ńv���t�@�C���̌݊������Ȃ��Ȃ邩������܂���B  
�����ɂ��肦�܂��B

## �g�p�\�ȃt�B���^�ƓK�p��

�t�B���^�ɂ��Ă͉��L�̏��ԂœK�p����܂��B

- �F��ԕϊ�
- nnedi
- �m�C�Y����(smooth)
- �m�C�Y����(knn)
- �m�C�Y����(pmd)
- ���T�C�Y
- unsharp
- �G�b�W���x������
- warpsharp
- �o���f�B���O�ጸ
- �F���␳

## �g�p���@

�܂��A�g�p����f�o�C�X��I�����Ă��������BiGPU / dGPU�ǂ�������p�\�ł��B

�E���� [clinfo] �{�^����OpenCL�̔F������GPU�f�o�C�X�̏ڍ׏����e�L�X�g�t�@�C���ɏo�͂ł��܂��B

![�f�o�C�X�I��](./data/clfilters_select_device.png)

���̌�A�K�p����t�B���^�Ƀ`�F�b�N�����A�p�����[�^�������s���Ă��������B

![�p�����[�^����](./data/clfilters_params.png)

## �����T�v

Aviutl�̓����t�H�[�}�b�g(YC48)����GPU�ň����₷��YUV444 16bit�ɕϊ������̂��AGPU�ɓ]�����ăt�B���^�������s���܂��B���̌�A�������ʂ�CPU�ɓ]�����AYC48�ɖ߂��ď��������������܂��B

YC48 �� YUV444 16bit �� GPU�ɓ]�� �� Vpp�t�B���^��GPU�ŘA������ �� CPU�ɓ]�� �� YUV444 16bit �� YC48

## GPU�t�B���^�̍�����

### GPU�t�B���^�̉ۑ�

GPU�ł̉��Z�����͍����ł����A������Ƃ�����GPU�t�B���^�������Ƃ͌���܂���B

- CPU-GPU�ԓ]�����x��  
  Aviutl�ł͊�{�I�ɂ̓t���[���̃f�[�^��CPU�ɂ��邽�߁AGPU�t�B���^��K�p����ꍇ�ACPU-GPU�Ԃ̒ʐM���K�v�ɂȂ�܂��B�Ƃ��낪����͂��Ȃ�x�������ŁA�]���J�n�̃��C�e���V���傫�����A�]���ɂ����Ԃ�������܂��B

- ��������GPU�ł̌v�Z�J�n���x���B  
  �����Ɍv�Z���s���Ă����CPU�ƈقȂ�AGPU�Ɍv�Z�𔭍s���Ă�����ۂɊJ�n�����܂łɂ���Ȃ�̒x��������܂��B

CPU-GPU�ԓ]����AGPU�̏����J�n�x���̉e���ŁA�v�Z�𒼗�ɕ��ׂ�ƁA�t�B���^�ɂ���Ă͕��ʂ�CPU�ł�AVX2�Ƃ��ōœK�������ق��������Ƃ������ƂɂȂ��Ă��܂��܂��BGPU�ł������ɂ��邽�߂ɂ́A

- CPU-GPU�ԓ]�������炷
- CPU-GPU�ԓ]����GPU�v�Z����s���Ď��s����

�Ȃǂ̑΍􂪕K�v�ɂȂ�܂��B

### ���{�ς݂̍œK��

#### CPU-GPU�ԓ]�������炷

������GPU�t�B���^����x�ɓK�p���邱�ƂŁACPU-GPU�ԓ]�����팸���Ă��܂��BGPU�t�B���^���ЂƂЂƂK�p����̂ɔ�ׁA�]���񐔂����炷���Ƃ��ł��܂��B

#### ����GPU�g�p����CPU-GPU�ԓ]���̍팸

Intel GPU�ȂǁA����GPU���g�p����ꍇ�AOpenCL��API��K�؂Ɏg�����Ƃ�CPU-GPU�ԓ]�����Ȃ����悤�ɂ��܂��� (������Zero Copy)�B

#### CPU-GPU�ԓ]����GPU�v�Z����s���Ď��s

�ۑ����ɂ̓t���[�����ǂ݂��邱�ƂŁACPU-GPU�ԓ]����GPU�v�Z����s���čs�����ƂŁACPU-GPU�ԓ]���⏈���J�n�̒x����}�����Ă��܂��B(�����̒P�����̂��߁A�ۑ����̂ݍs���Ă���A�ҏW���͍s���Ă��܂���B)

#### �������m�ہE�Ċm�ۂ̍팸

GPU�̃������m�ۂ�CPU�ȏ�ɒx���̂ŁA�Ȃ�ׂ��m�ۂ������������g���܂킷�悤�ɍH�v���܂����B

#### �s�v�ȓ����̍폜

�܂�GPU���V��ł��鎞�Ԃ������󋵂������̂ŁAVTune�Ȃǂ��g���āAOpenCL API�̌Ă΂���Ȃǂ��`�F�b�N���܂����B���\�[�X�̉���R��ɂ��A���ʂȓ������������Ă��܂��Ă����̂��������č��������܂����B

## �ۑ�

clfilters �ɂ͉��L�̉ۑ肪����܂��B

- NVIDIA��GPU���A[cl_khr_image2d_from_buffer](https://www.khronos.org/registry/OpenCL/sdk/3.0/docs/man/html/cl_khr_image2d_from_buffer.html) �Ƃ���KHR�g�����T�|�[�g����Ȃ����Ŗ��ʂɃ������R�s�[����������B  
  OpenCL 2.0�ł��̊g���͕W���ɂȂ����̂ŁA���낢��ȃt�B���^�� cl_khr_image2d_from_buffer ���肫�Ŏ������Ă����̂ł����A
  OpenCL 3.0�ŕW������O��Ă��܂��ANVIDIA GPU�ł͑Ή����Ă��Ȃ��悤�ł�(Intel/AMD�͑Ή�)�B
  CUDA�ł��قړ������Ƃł���̂ɂȂ�ŃT�|�[�g���Ȃ��́c�B�߂��݁B

- ���ԕ����ɎQ�Ƃ���t�B���^�ɖ��Ή��B  
  vpp-convolution3d �ȂǁB��������₱�����̂Ō����蒆�ł��B
  
  
## �t�B���^

### �F��ԕϊ�  
�w��̐F��ԕϊ����s���B

**�p�����[�^**
- matrix=&lt;from&gt;:&lt;to&gt;  
  
```
  bt709, smpte170m, bt470bg, smpte240m, YCgCo, fcc, GBR, bt2020nc, bt2020c, auto
```

- colorprim=&lt;from&gt;:&lt;to&gt;  
```
  bt709, smpte170m, bt470m, bt470bg, smpte240m, film, bt2020, auto
```

- transfer=&lt;from&gt;:&lt;to&gt;  
```
  bt709, smpte170m, bt470m, bt470bg, smpte240m, linear,
  log100, log316, iec61966-2-4, iec61966-2-1,
  bt2020-10, bt2020-12, smpte2084, arib-std-b67, auto
```

- range=&lt;from&gt;:&lt;to&gt;  
```
  tv, full, auto
```


- hdr2sdr  
  tone-mapping���w�肵��HDR����SDR�ւ̕ϊ����s���B 
  
  - none  (�f�t�H���g)  
    hdr2sdr�̏������s���Ȃ��B

  - hable    
    �����ƈÕ��̃f�B�e�[���̗������o�����X�悭�ۂ��Ȃ���ϊ�����B(�������A���Â߂ɂȂ�)

  - mobius  
    �Ȃ�ׂ���ʂ̖��邳��R���g���X�g���ێ������ϊ����s�����A�����̃f�B�e�[�����Ԃ��\��������B
  
  - reinhard  
      
  - bt2390  
    BT.2390�ŋK�肳���tone mapping�B


- ���̓s�[�N  (�f�t�H���g: 1000)  

- �ڕW�P�x  (�f�t�H���g: 100)  


### nnedi  
nnedi�ɂ��C���^���������s���B��{�I�ɂ͕Е��t�B�[���h�͎̂ĂāA�����Е��̃t�B�[���h����
�j���[�����l�b�g���g���ė֊s��␳���Ȃ���t���[�����č\�z���邱�ƂŃC���^���������邪�A�ƂĂ��d���c�B

**�p�����[�^**
- field  
  �C���^�������̕��@�B
  - top  
    �g�b�v�t�B�[���h�ێ�
  - bottom  
    �{�g���t�B�[���h�ێ�

- nns  (�f�t�H���g: 32)  
  �j���[�����l�b�g�̃j���[�������B
  - 16, 32, 64, 128, 256

- nsize  (�f�t�H���g: 32x4)  
  �j���[�����l�b�g���Q�Ƃ���ߖT�u���b�N�̃T�C�Y�B
  - 8x6, 16x6, 32x6, 48x6, 8x4, 16x4, 32x4

- �i��  (�f�t�H���g: fast)  

  - fast

  - slow  
    slow�ł�fast�̃j���[�����l�b�g�̏o�͂ɁA�����ЂƂ�
    �j���[�����l�b�g�̏o�͂��u�����h���ĕi�����グ��(���R���̕�����ɒx��)�B

- �O���� (�f�t�H���g: new_block)  
  ���O�ɑO�������s���A�P���ȕ�Ԃōς܂����A�j���[�����l�b�g�ł̕␳���s�������肷��B
  ��{�I�ɂ̓G�b�W�ߖT���j���[�����l�b�g�ł̕␳�̑ΏۂƂȂ�A�j���[�����l�b�g���g���p�x�������邱�Ƃŏ����������ɂȂ�B
  
  - none  
    �O�������s�킸�A���ׂĂ�pixel���j���[�����l�b�g�ōč\������B

  - original
  - new  
    �O�������s���A�K�v�ȂƂ���̂݃j���[�����l�b�g�ł̕␳���s���悤�ɂ���Boriginal��new�͕������قȂ�Bnew�̂ق��������Ȃ�X���ɂ���B

  - original_block
  - new_block  
    original/new��GPU�œK���ŁBpixel�P�ʂ̔���̑���Ƀu���b�N�P�ʂ̔�����s���B

- errortype (�f�t�H���g: abs)  
  �j���[�����l�b�g�̏d�݃p�����[�^��I������B
  - abs  
    ��Ό덷���ŏ��ɂ���悤�w�K���ꂽ�d�݂�p����B
  - square  
    ���덷���ŏ��ɂ���悤�w�K���ꂽ�d�݂�p����B
  

### �m�C�Y���� (smooth)  

**�p�����[�^**
- �i��  (default=3, 1-6)  
  �����̕i���B�l���傫���قǍ����x�����x���Ȃ�B

- QP  (default=12, 1 - 63)    
  �t�B���^�̋����B
    
  
### �m�C�Y���� (knn)  

**�p�����[�^**
- �K�p���a  (default=3, 1-5)  

- ����  (default=8, 0 - 100)    
  �t�B���^�̋����B

- �u�����h�x;  (default=20, 0 - 100)  
  �m�C�Y�����s�N�Z���ւ̃I���W�i���s�N�Z���̃u�����h�x�����B

- �u�����h�   (default=80, 0 - 100)  
  �G�b�W���o��臒l�B


### �m�C�Y���� (pmd)  
������pmd�@�ɂ��m�C�Y�����B��߂̃m�C�Y�������s�������Ƃ��Ɏg�p����B

**�p�����[�^**
- �K�p��  (default=2, 1- )  
  �K�p�񐔁B�f�t�H���g��2�B

- ����  (default=100, 0-100)  
  �t�B���^�̋����B

- 臒l  (default=100, 0-255)  
  �t�B���^�̗֊s���o��臒l�B�������قǗ֊s��ێ�����悤�ɂȂ邪�A�t�B���^�̌��ʂ���܂�B



### ���T�C�Y  
���T�C�Y�̃A���S���Y�����w��ł���B

| �I�v�V������ | ���� |
|:---|:---|
| spline16 | 4x4 Spline��� |
| spline36 | 6x6 Spline��� |
| spline64 | 8x8 Spline��� |
| lanczos2 | 4x4 lanczos��� |
| lanczos3 | 6x6 lanczos��� |
| lanczos4 | 8x8 lanczos��� |
| bilinear | ���`��� |


### unsharp  
�֊s�E�f�B�e�[�������p�̃t�B���^�B

**�p�����[�^**
- �͈� (default=3, 1-9)  
  �֊s�E�f�B�e�[�����o�͈̔́B���傫�Ȓl�Ƃ��邱�ƂŁA���L���͈͂̃f�B�e�[���ɔ������ċ�����������悤�ɂȂ�B

- ���� (default=0, 0-100)  
  �֊s�E�f�B�e�[�������̋����B���傫�Ȓl�Ƃ��邱�ƂŁA����������������B

- 臒l (default=10, 0-255)  
  �֊s�E�f�B�e�[�����o��臒l�B臒l�ȏ�̍��ق������f�ɑ΂��āA�֊s�������s���B


### �G�b�W���x������  
�֊s�����p�̃t�B���^�B

**�p�����[�^**
- ���� (default=5, -31 - 31)  
  �֊s�����̋����B���傫�Ȓl�Ƃ��邱�ƂŁA�֊s���������͂ɂȂ�B

- 臒l (default=20, 0 - 255)  
  �֊s�������s��Ȃ��悤�ɂ���m�C�Y��臒l�B���傫�Ȓl�قǑ傫�ȋP�x�̕ω����m�C�Y�Ƃ��Ĉ����悤�ɂȂ�B

- �� (default=0, 0-31)  
  �֊s�̍��������ɂ��āA��荕���V���[�g�����ė֊s����������悤�ɂ���B

- w�� (default=0, 0-31)  
  �֊s�̔��������ɂ��āA��蔒���V���[�g�����ė֊s����������悤�ɂ���B


### warpsharp  
�א����t�B���^�B�֊s�����p�̃t�B���^�B

**�p�����[�^**
- �u���[  (default=2)  
  blur�������s���񐔁B�l��������قǃt�B���^�̋��x����܂�B

- 臒l (default=128, 0 - 255)  
  �֊s���o��臒l�B�l��������قǃt�B���^�̋��x�����܂�B
  
- �[�x (default=16, -128 - 128)  
  warp�̐[�x�B�l��������قǃt�B���^�̋��x�����܂�B

- �}�X�N�T�C�Y  (default=�I�t)  
  - �I�t ... 13x13��blur�������s���B
  - �I�� ... 5x5��blur�������s���B��荂�i�������Ablur�񐔂𑽂߂ɂ���K�v������B
  
- �F���}�X�N  (default=�I�t)  
  �F���̏������@�̎w��B
  - �I�t ... �P�x�x�[�X�̗֊s���o��F�������ɂ��K�p����B
  - �I�� ... �e�F�������ɂ��Ă��ꂼ��֊s���o���s���B

### --vpp-deband [&lt;param1&gt;=&lt;value1&gt;][,&lt;param2&gt;=&lt;value2&gt;],...

**�p�����[�^**
- range=&lt;int&gt; (default=15, 0-127)  
  �ڂ����͈́B���͈͓̔��̋ߖT��f����T���v�������A�u���[�������s���B

- sample=&lt;int&gt; (default=1, 0-2)  
  - �ݒ�l�F0  
    ����1��f���Q�Ƃ��A���̉�f�l���ێ������܂܏������s���B

  - �ݒ�l�F1  
    ����1��f�Ƃ��̓_�Ώ̉�f�̌v2��f���Q�Ƃ��A�u���[�������s���B

  - �ݒ�l�F2  
    ����2��f�Ƃ��̓_�Ώ̉�f�̌v4��f���Q�Ƃ��A�u���[�������s���B

- thre=&lt;int&gt; (�ꊇ�ݒ�)
- thre_y=&lt;int&gt; (default=15, 0-31)
- thre_cb=&lt;int&gt; (default=15, 0-31)
- thre_cr=&lt;int&gt; (default=15, 0-31)  
  y,cb,cr �e������臒l�B���̒l�������ƊK����т����炷����ŁA�ׂ������Ȃǂ��ׂ�₷���Ȃ�B

- dither=&lt;int&gt; (�ꊇ�ݒ�)
- dither_y=&lt;int&gt; (default=15, 0-31)
- dither_c=&lt;int&gt; (default=15, 0-31)  
  y������ cb+cr�����̃f�B�U�̋����B

- seed=&lt;int&gt;  
  �����V�[�h�̕ύX�B (default=1234)

- blurfirst (default=off)  
  �u���[�������ɂ��邱�ƂŃf�B�U���x�����炵�A�K����т������f�ނł̌��ʂ��グ��B
  �S�̓I�ɕ���p�������Ȃ�ׂ��������ׂ�₷���Ȃ�B

- rand_each_frame (default=off)  
  ���t���[���g�p���闐����ύX����B

### --vpp-tweak [&lt;param1&gt;=&lt;value1&gt;][,&lt;param2&gt;=&lt;value2&gt;],...

**�p�����[�^**
- brightness=&lt;float&gt; (default=0.0, -1.0 - 1.0)  

- contrast=&lt;float&gt; (default=1.0, -2.0 - 2.0)  

- gamma=&lt;float&gt; (default=1.0, 0.1 - 10.0)  

- saturation=&lt;float&gt; (default=1.0, 0.0 - 3.0)  

- hue=&lt;float&gt; (default=0.0, -180 - 180)  

```
��:
--vpp-tweak brightness=0.1,contrast=1.5,gamma=0.75
```


## �R���p�C����

VC++ 2022 Community



