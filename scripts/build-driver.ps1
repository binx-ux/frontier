# Build FrontierDrv.sys (WDK required)
$root = Split-Path -Parent $PSScriptRoot
$pf86 = ${env:ProgramFiles(x86)}

function Find-MsBuild {
    $candidates = @(
        "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    )
    foreach ($p in $candidates) {
        if (Test-Path $p) { return $p }
    }
    return $null
}

function Get-WdkVersion {
    $includes = Get-ChildItem "$pf86\Windows Kits\10\Include" -Directory -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending
    foreach ($ver in $includes) {
        $km = Join-Path $ver.FullName "km\ntddk.h"
        if (Test-Path $km) { return $ver.Name }
    }
    return $null
}

function Get-MsvcRoot {
    $roots = @(
        "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC"
    )
    foreach ($rootPath in $roots) {
        if (-not (Test-Path $rootPath)) { continue }
        $latest = Get-ChildItem $rootPath -Directory | Sort-Object Name -Descending | Select-Object -First 1
        if ($latest) { return $latest.FullName }
    }
    return $null
}

function Build-DriverManual {
    param(
        [string]$WdkVer,
        [string]$MsvcRoot
    )

    $srcDir = Join-Path $root "kernel\driver\frontier_drv"
    $outDir = Join-Path $root "kernel\driver"
    $objDir = Join-Path $srcDir "x64\Release"
    $obj = Join-Path $objDir "frontier_drv.obj"
    $sys = Join-Path $outDir "FrontierDrv.sys"

    New-Item -ItemType Directory -Force -Path $objDir | Out-Null

    $cl = Join-Path $MsvcRoot "bin\Hostx64\x64\cl.exe"
    $link = Join-Path $MsvcRoot "bin\Hostx64\x64\link.exe"
    if (-not (Test-Path $cl) -or -not (Test-Path $link)) {
        Write-Error "cl.exe or link.exe not found under $MsvcRoot"
        return $false
    }

    $kmInc = Join-Path $pf86 "Windows Kits\10\Include\$WdkVer\km"
    $sharedInc = Join-Path $pf86 "Windows Kits\10\Include\$WdkVer\shared"
    $ucrtInc = Join-Path $pf86 "Windows Kits\10\Include\$WdkVer\ucrt"
    $msvcInc = Join-Path $MsvcRoot "include"
    $kmLib = Join-Path $pf86 "Windows Kits\10\Lib\$WdkVer\km\x64"
    $msvcLib = Join-Path $MsvcRoot "lib\x64"
    $src = Join-Path $srcDir "frontier_drv.c"

    Write-Host "Manual WDK build (cl/link)..." -ForegroundColor Cyan
    & $cl /nologo /c /kernel /W3 /O2 /GS- /Gy /Zc:wchar_t- /D_AMD64_ /D_WIN64 /DNDEBUG `
        /I"$kmInc" /I"$sharedInc" /I"$ucrtInc" /I"$msvcInc" /Fo"$obj" $src
    if ($LASTEXITCODE -ne 0) { return $false }

    & $link /nologo /DRIVER:WDM /SUBSYSTEM:NATIVE,10.00 /ENTRY:DriverEntry /MACHINE:X64 /OUT:"$sys" `
        /NODEFAULTLIB /LIBPATH:"$kmLib" /LIBPATH:"$msvcLib" $obj `
        ntoskrnl.lib hal.lib wmilib.lib BufferOverflowFastFailK.lib
    return ($LASTEXITCODE -eq 0 -and (Test-Path $sys))
}

$wdkVer = Get-WdkVersion
if (-not $wdkVer) {
    Write-Host "WDK kernel headers not found." -ForegroundColor Yellow
    Write-Host "Install: winget install Microsoft.WindowsWDK.10.0.26100"
    exit 1
}

$out = Join-Path $root "kernel\driver\FrontierDrv.sys"
$built = $false

$msbuild = Find-MsBuild
if ($msbuild) {
    $proj = Join-Path $root "kernel\driver\frontier_drv\frontier_drv.vcxproj"
    Write-Host "Trying MSBuild driver project (WDK $wdkVer)..." -ForegroundColor Cyan
    & $msbuild $proj /p:Configuration=Release /p:Platform=x64 /p:WindowsTargetPlatformVersion=$wdkVer /v:minimal 2>$null
    if ($LASTEXITCODE -eq 0 -and (Test-Path $out)) {
        $built = $true
    } else {
        Write-Host "MSBuild toolset unavailable - falling back to manual cl/link." -ForegroundColor Yellow
    }
}

if (-not $built) {
    $msvc = Get-MsvcRoot
    if (-not $msvc) {
        Write-Error "MSVC toolchain not found."
        exit 1
    }
    $built = Build-DriverManual -WdkVer $wdkVer -MsvcRoot $msvc
}

if ($built -and (Test-Path $out)) {
    Write-Host "Driver built: $out" -ForegroundColor Green
    Write-Host "Size: $((Get-Item $out).Length) bytes" -ForegroundColor Green
} else {
    Write-Error "FrontierDrv.sys build failed."
    exit 1
}
