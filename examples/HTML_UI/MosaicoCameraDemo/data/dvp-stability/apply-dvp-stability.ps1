# SPDX-License-Identifier: Apache-2.0
[CmdletBinding()]
param(
  [Parameter(Mandatory=$true)][string]$SdkRoot,
  [Parameter(Mandatory=$true)][string]$GccRoot,
  [switch]$BuildOnly
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
function Require-File([string]$Path) {
  if (!(Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Required file missing: $Path" }
}
function File-Sha([string]$Path) { return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
function Run-Native([string]$Executable, [string[]]$Arguments) {
  $outputLines = @(& $Executable @Arguments 2>&1)
  if ($LASTEXITCODE -ne 0) { throw "Native command failed ($LASTEXITCODE): $Executable $($Arguments -join ' '); $($outputLines -join [Environment]::NewLine)" }
  return $outputLines
}
function Global-Symbols([string]$ObjectPath) {
  $result = @{}
  foreach ($line in @(Run-Native $script:nmExe @('-g','--defined-only',$ObjectPath))) {
    if ("$line" -match '^\s*[0-9a-fA-F]+\s+[A-Za-z]\s+(\S+)\s*$') { $result[$Matches[1]] = $true }
  }
  return $result
}
$SdkRoot = (Resolve-Path -LiteralPath $SdkRoot).Path
$GccRoot = (Resolve-Path -LiteralPath $GccRoot).Path
$packageRoot = $PSScriptRoot
$manifestPath = Join-Path $packageRoot 'SOURCE-MANIFEST.json'
Require-File $manifestPath
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ($manifest.idfCommit -ne '94639ab7251fc8230ca725df638c9a57dbb17153' -or
    $manifest.bspCommit -ne 'e110720fc6eb1f1d208fcbc42f7881ca8ce782d6') { throw 'Unsupported source manifest.' }
foreach ($file in $manifest.files) {
  $path = Join-Path $packageRoot $file.path
  Require-File $path
  if ((File-Sha $path) -ne $file.sha256) { throw "Package source checksum mismatch: $($file.path)" }
}
$definesPath = Join-Path $SdkRoot 'flags\defines'
$cFlagsPath = Join-Path $SdkRoot 'flags\c_flags'
$includesPath = Join-Path $SdkRoot 'flags\includes'
$configPath = Join-Path $SdkRoot 'qio_opi\include\sdkconfig.h'
$archivePath = Join-Path $SdkRoot 'lib\libesp_driver_cam.a'
$gccExe = Join-Path $GccRoot 'bin\riscv32-esp-elf-gcc.exe'
$arExe = Join-Path $GccRoot 'bin\riscv32-esp-elf-ar.exe'
$script:nmExe = Join-Path $GccRoot 'bin\riscv32-esp-elf-nm.exe'
foreach ($path in @($definesPath,$cFlagsPath,$includesPath,$configPath,$archivePath,$gccExe,$arExe,$script:nmExe)) { Require-File $path }
$defines = Get-Content -LiteralPath $definesPath -Raw
if ($defines -notmatch 'IDF_VER=[^\s]*v6\.1-803-g94639ab7251') {
  throw 'Only the exact Arduino ESP-IDF v6.1-803-g94639ab7251 SDK is supported. No archive changed.'
}
$config = Get-Content -LiteralPath $configPath -Raw
foreach ($name in @('CONFIG_IDF_TARGET_ESP32S31','CONFIG_SPIRAM','CONFIG_ESP_VIDEO_ENABLE_DVP_VIDEO_DEVICE')) {
  if ($config -notmatch ('(?m)^\s*#define\s+' + $name + '\s+1\s*$')) { throw "Required S31 SDK option missing: $name" }
}
$compilerMachine = ("$(Run-Native $gccExe @('-dumpmachine'))").Trim()
$compilerVersion = ("$(Run-Native $gccExe @('-dumpfullversion'))").Trim()
if ($compilerMachine -ne 'riscv32-esp-elf' -or $compilerVersion -ne '15.2.0') {
  throw "Expected official RISC-V GCC 15.2.0, got $compilerMachine / $compilerVersion."
}
$markerPath = Join-Path $SdkRoot 'lib\nice-gfx-mosaico-dvp-stability.json'
$manifestHash = File-Sha $manifestPath
if (Test-Path -LiteralPath $markerPath) {
  $marker = Get-Content -LiteralPath $markerPath -Raw | ConvertFrom-Json
  if ($marker.sourceManifestSha256 -eq $manifestHash -and
      (File-Sha $archivePath) -eq $marker.patchedArchiveSha256) {
    Require-File $marker.backupPath
    if ((File-Sha $marker.backupPath) -ne $marker.originalArchiveSha256) { throw 'Original archive backup checksum mismatch.' }
    Write-Output 'Mosaico DVP stability adapter already installed and verified; no files changed.'
    Write-Output "Archive: $archivePath"
    Write-Output "Backup: $($marker.backupPath)"
    exit 0
  }
  throw 'An adapter marker exists but source/archive does not match. Restore its backup or use a clean supported SDK.'
}
$originalHash = File-Sha $archivePath
if ($originalHash -ne $manifest.originalArchiveSha256) {
  throw "Camera archive differs from the verified exact SDK. Refusing to overwrite it. Actual SHA256: $originalHash"
}
$originalMembers = @(Run-Native $arExe @('t',$archivePath))
foreach ($member in @('esp_cam_ctlr_dvp_cam.c.obj','esp_cam_ctlr_dvp_gdma.c.obj')) {
  if (@($originalMembers | Where-Object { "$_".Trim() -eq $member }).Count -ne 1) {
    throw "Expected exactly one archive member: $member"
  }
}
$originalSymbols = Global-Symbols $archivePath
$buildRoot = Join-Path ([IO.Path]::GetTempPath()) ('nice-gfx-mosaico-dvp-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $buildRoot | Out-Null
$sourceRoot = Join-Path $packageRoot 'patched\components\esp_driver_cam'
$compileArguments = @(
  ('@' + $definesPath), ('@' + $cFlagsPath),
  '-iprefix', ((Join-Path $SdkRoot 'include') + '\'),
  ('@' + $includesPath),
  '-I', (Join-Path $SdkRoot 'qio_opi\include'),
  '-I', (Join-Path $sourceRoot 'dvp\private_include'),
  '-isystem', (Join-Path $GccRoot 'riscv32-esp-elf\include'),
  '-isystem', (Join-Path $GccRoot 'lib\gcc\riscv32-esp-elf\15.2.0\include'),
  '-isystem', (Join-Path $GccRoot 'lib\gcc\riscv32-esp-elf\15.2.0\include-fixed'),
  '-Os', '-c'
)
$objects = @()
foreach ($stem in @('esp_cam_ctlr_dvp_cam','esp_cam_ctlr_dvp_gdma')) {
  $object = Join-Path $buildRoot ($stem + '.c.obj')
  Run-Native $gccExe ($compileArguments + @((Join-Path $sourceRoot ('dvp\src\' + $stem + '.c')),'-o',$object)) | Write-Output
  $objects += $object
}
$candidatePath = Join-Path $buildRoot 'libesp_driver_cam.a'
Copy-Item -LiteralPath $archivePath -Destination $candidatePath
Run-Native $arExe (@('rcs',$candidatePath) + $objects) | Write-Output
$candidateMembers = @(Run-Native $arExe @('t',$candidatePath))
if ($candidateMembers.Count -ne $originalMembers.Count) { throw 'Archive member count changed unexpectedly.' }
foreach ($member in $originalMembers) {
  if (@($candidateMembers | Where-Object { "$_".Trim() -eq "$member".Trim() }).Count -ne 1) {
    throw "Archive member was lost or duplicated: $member"
  }
}
$candidateSymbols = Global-Symbols $candidatePath
foreach ($name in $originalSymbols.Keys) {
  if (!$candidateSymbols.ContainsKey($name)) { throw "Existing global driver symbol was lost: $name" }
}
$patchedHash = File-Sha $candidatePath
if ($BuildOnly) {
  Write-Output 'Build-only validation passed; SDK archive was not changed.'
  Write-Output "Candidate archive: $candidatePath"
  Write-Output "Candidate SHA256: $patchedHash"
  exit 0
}
if ((File-Sha $archivePath) -ne $originalHash) { throw 'SDK archive changed while compiling; refusing concurrent overwrite.' }
$operationId = [guid]::NewGuid().ToString('N')
$libraryDirectory = Split-Path $archivePath -Parent
$backupPath = Join-Path $libraryDirectory ('libesp_driver_cam.a.before-mosaico-dvp-94639ab-' + $operationId + '.bak')
$stagePath = Join-Path $libraryDirectory ('libesp_driver_cam.a.mosaico-dvp-stage-' + $operationId)
Copy-Item -LiteralPath $candidatePath -Destination $stagePath
[IO.File]::Replace($stagePath,$archivePath,$backupPath)
if ((File-Sha $archivePath) -ne $patchedHash -or (File-Sha $backupPath) -ne $originalHash) {
  throw "Archive replacement checksum failed. Original backup: $backupPath"
}
$marker = [ordered]@{
  adapter='nice_gfx optional ESP-Mosaico DVP stability adapter'
  idfCommit=$manifest.idfCommit; bspCommit=$manifest.bspCommit
  sourceManifestSha256=$manifestHash
  compilerMachine=$compilerMachine; compilerVersion=$compilerVersion
  sdkRoot=$SdkRoot; backupPath=$backupPath
  originalArchiveSha256=$originalHash; patchedArchiveSha256=$patchedHash
  completedAtUtc=[DateTime]::UtcNow.ToString('o')
}
$marker | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $markerPath -Encoding UTF8
Write-Output 'Mosaico DVP stability adapter installed; other archive members and global symbols retained.'
Write-Output "Archive: $archivePath"
Write-Output "Original backup: $backupPath"
Write-Output "Archive SHA256: $patchedHash"
Write-Output "Build files: $buildRoot"
Write-Output 'Rebuild the sketch with a fresh build directory so it relinks this archive.'

