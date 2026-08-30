# Build FRONTIER kernel client (ReleaseKernel|x64)
$root = Split-Path -Parent $PSScriptRoot
$msbuild = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
if (-not (Test-Path $msbuild)) {
    $msbuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
}
if (-not (Test-Path $msbuild)) {
    Write-Error "MSBuild not found. Install Visual Studio with C++ desktop workload."
    exit 1
}

& $msbuild (Join-Path $root "External\External.vcxproj") /p:Configuration=ReleaseKernel /p:Platform=x64 /v:minimal
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$out = Join-Path $root "kernel\Frontier.exe"
if (Test-Path $out) {
    Write-Host "Kernel client built: $out"
} else {
    Write-Error "Build finished but Frontier.exe missing at $out"
    exit 1
}

New-Item -ItemType Directory -Force -Path (Join-Path $root "kernel\driver") | Out-Null
Write-Host "Place FrontierDrv.sys in kernel\driver\ (see kernel\driver\README.md)"
