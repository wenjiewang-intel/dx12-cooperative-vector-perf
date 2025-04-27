REM Set the working directory to the directory of the batch file
cd /d %~dp0

@REM .\dxc.exe -T cs_6_5 -E main -Fo .\VectorAdd.cso .\VectorAdd.hlsl
@REM copy .\VectorAdd.cso ..\out\build\x64-Debug\

@REM C:\Users\wenjiew2\LocalFiles\1009-GPU\Task-13-ClientGPU\00-repos\DirectXShaderCompiler\out\build\x64-Debug\bin\dxc.exe -T cs_6_9 -E main -Fo .\CoopVectorMulAdd.cso .\CoopVectorMulAdd.hlsl


.\dxc.exe -T cs_6_9 -enable-16bit-types -E main -Fo .\CoopVectorMulAdd.cso .\CoopVectorMulAdd.hlsl
copy .\CoopVectorMulAdd.cso ..\out\build\x64-Debug\

.\dxc.exe -T cs_6_0 -E main -Fo .\VectorMulAdd.cso .\VectorMulAdd.hlsl
copy .\VectorMulAdd.cso ..\out\build\x64-Debug\