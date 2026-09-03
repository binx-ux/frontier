# Export MIT-licensed FRONTIER base to a public-ready folder.
# Usage:
#   .\scripts\export-public-base.ps1
#   .\scripts\export-public-base.ps1 -OutputDir D:\repos\frontier-base
#   .\scripts\export-public-base.ps1 -DryRun

param(
    [string]$OutputDir = (Join-Path (Split-Path $PSScriptRoot -Parent) "..\external-base"),
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"
$Root = Split-Path $PSScriptRoot -Parent
$OutputDir = [System.IO.Path]::GetFullPath($OutputDir)

$CopyItems = @(
    @{ Src = "kernel";               Dst = "kernel" },
    @{ Src = "offsets";              Dst = "offsets" },
    @{ Src = "base";                 Dst = "base" },
    @{ Src = "docs";                 Dst = "docs" },
    @{ Src = "LICENSE";              Dst = "LICENSE" },
    @{ Src = "LICENSE-PROPRIETARY.md"; Dst = "LICENSE-PROPRIETARY.md" },
    @{ Src = "WARNING.md";           Dst = "WARNING.md" },
    @{ Src = "SECURITY.md";           Dst = "SECURITY.md" },
    @{ Src = "scripts\sync-offsets.js"; Dst = "scripts\sync-offsets.js" },
    @{ Src = "scripts\build-kernel.ps1"; Dst = "scripts\build-kernel.ps1" },
    @{ Src = "scripts\export-public-base.ps1"; Dst = "scripts\export-public-base.ps1" }
)

$CopyFiles = @(
    @{ Src = "External\src\memory\memory.cpp";      Dst = "External\src\memory\memory.cpp" },
    @{ Src = "External\src\memory\memory.h";        Dst = "External\src\memory\memory.h" },
    @{ Src = "External\src\memory\driver_link.cpp"; Dst = "External\src\memory\driver_link.cpp" },
    @{ Src = "External\src\memory\driver_link.h";   Dst = "External\src\memory\driver_link.h" },
    @{ Src = "External\src\memory\luck.asm";        Dst = "External\src\memory\luck.asm" },
    @{ Src = "External\src\sdk\sdk.h";              Dst = "External\src\sdk\sdk.h" },
    @{ Src = "External\src\sdk\offsets.h";        Dst = "External\src\sdk\offsets.h" },
    @{ Src = "External\src\sdk\w2s.h";              Dst = "External\src\sdk\w2s.h" },
    @{ Src = "External\src\sdk\scanner.cpp";       Dst = "External\src\sdk\scanner.cpp" },
    @{ Src = "External\src\sdk\scanner.h";          Dst = "External\src\sdk\scanner.h" },
    @{ Src = "External\src\sdk\math.h";             Dst = "External\src\sdk\math.h" },
    @{ Src = "External\src\sdk\structs.h";         Dst = "External\src\sdk\structs.h" },
    @{ Src = "External\src\sdk\window_manager.h";  Dst = "External\src\sdk\window_manager.h" },
    @{ Src = "External\src\core\globals\globals.h"; Dst = "External\src\core\globals\globals.h" },
    @{ Src = "External\src\core\debug_log.h";      Dst = "External\src\core\debug_log.h" },
    @{ Src = "External\src\core\tp_handler\tp_handler.cpp"; Dst = "External\src\core\tp_handler\tp_handler.cpp" },
    @{ Src = "External\src\core\tp_handler\tp_handler.h";   Dst = "External\src\core\tp_handler\tp_handler.h" }
)

$ExcludePatterns = @(
    "*.exe", "*.dll", "*.sys", "*.pdb", "*.obj", "*.iobj", "*.ipdb",
    "x64", "Debug", "Release", ".vs", "Thumbs.db"
)

function Should-SkipPath([string]$Path) {
    foreach ($pat in $ExcludePatterns) {
        if ($Path -like "*\$pat\*" -or $Path -like "*\$pat" -or (Split-Path $Path -Leaf) -like $pat) {
            return $true
        }
    }
    return $false
}

function Copy-Tree([string]$RelativeSrc, [string]$RelativeDst) {
    $srcPath = Join-Path $Root $RelativeSrc
    $dstPath = Join-Path $OutputDir $RelativeDst
    if (-not (Test-Path $srcPath)) {
        Write-Warning "Skip missing: $RelativeSrc"
        return
    }
    if ($DryRun) {
        Write-Host "[dry-run] tree $RelativeSrc -> $RelativeDst"
        return
    }
    if (Test-Path $dstPath) { Remove-Item $dstPath -Recurse -Force }
    $null = New-Item -ItemType Directory -Path (Split-Path $dstPath -Parent) -Force
    robocopy $srcPath $dstPath /E /NFL /NDL /NJH /NJS /NC /NS /NP /XD x64 Debug Release .vs |
        Out-Null
    if ($LASTEXITCODE -ge 8) { throw "robocopy failed ($LASTEXITCODE) for $RelativeSrc" }
}

function Copy-One([string]$RelativeSrc, [string]$RelativeDst) {
    $srcPath = Join-Path $Root $RelativeSrc
    $dstPath = Join-Path $OutputDir $RelativeDst
    if (-not (Test-Path $srcPath)) {
        Write-Warning "Skip missing: $RelativeSrc"
        return
    }
    if (Should-SkipPath $srcPath) {
        Write-Warning "Skip excluded: $RelativeSrc"
        return
    }
    if ($DryRun) {
        Write-Host "[dry-run] file $RelativeSrc -> $RelativeDst"
        return
    }
    $null = New-Item -ItemType Directory -Path (Split-Path $dstPath -Parent) -Force
    Copy-Item $srcPath $dstPath -Force
}

Write-Host "FRONTIER public base export"
Write-Host "  Source: $Root"
Write-Host "  Output: $OutputDir"
if ($DryRun) { Write-Host "  Mode:   dry-run" }

if (-not $DryRun) {
    if (Test-Path $OutputDir) { Remove-Item $OutputDir -Recurse -Force }
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
}

foreach ($item in $CopyItems) {
    $srcPath = Join-Path $Root $item.Src
    if (Test-Path $srcPath -PathType Leaf) {
        Copy-One $item.Src $item.Dst
    } else {
        Copy-Tree $item.Src $item.Dst
    }
}
foreach ($file in $CopyFiles) { Copy-One $file.Src $file.Dst }

$publicGitignore = @"
.vs/
**/x64/
**/Debug/
**/Release/
*.exe
*.dll
*.sys
*.pdb
*.obj
.env
.env.*
**/secrets.h
"@

if (-not $DryRun) {
    $readmeSrc = Join-Path $Root "base\PUBLIC_README.md"
    if (Test-Path $readmeSrc) {
        Copy-Item $readmeSrc (Join-Path $OutputDir "README.md") -Force
    }
    Set-Content -Path (Join-Path $OutputDir ".gitignore") -Value $publicGitignore -Encoding UTF8
    Write-Host "Done. Review then: cd `"$OutputDir`" && git add . && git commit && git push"
} else {
    Write-Host "Dry-run complete."
}
