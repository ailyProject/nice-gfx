[CmdletBinding()]
param(
  [Parameter(Mandatory=$true)][string]$SdkRoot,
  [Parameter(Mandatory=$true)][string]$GccRoot
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
function Defined-Symbols([string]$ObjectPath) {
  $result = @{}
  foreach ($line in @(Run-Native $script:nmExe @('--defined-only', $ObjectPath))) {
    if ("$line" -match '^\s*[0-9a-fA-F]+\s+[A-Za-z]\s+(\S+)\s*$') { $result[$Matches[1]] = $true }
  }
  return $result
}

$SdkRoot = (Resolve-Path -LiteralPath $SdkRoot).Path
$GccRoot = (Resolve-Path -LiteralPath $GccRoot).Path
$packageRoot = $PSScriptRoot
$provenancePath = Join-Path $packageRoot 'PROVENANCE.json'
Require-File $provenancePath
$provenance = Get-Content -LiteralPath $provenancePath -Raw | ConvertFrom-Json
if ($provenance.componentVersion -ne '2.6.0') { throw 'This adapter package only supports esp_cam_sensor 2.6.0.' }
foreach ($file in $provenance.files) {
  $path = Join-Path $packageRoot $file.path
  Require-File $path
  if ((File-Sha $path) -ne $file.sha256) { throw "Package source checksum mismatch: $($file.path)" }
}

$definesPath = Join-Path $SdkRoot 'flags\defines'
$cFlagsPath = Join-Path $SdkRoot 'flags\c_flags'
$includesPath = Join-Path $SdkRoot 'flags\includes'
$configPath = Join-Path $SdkRoot 'qio_opi\include\sdkconfig.h'
$archivePath = Join-Path $SdkRoot 'lib\libespressif__esp_cam_sensor.a'
$gccExe = Join-Path $GccRoot 'bin\riscv32-esp-elf-gcc.exe'
$arExe = Join-Path $GccRoot 'bin\riscv32-esp-elf-ar.exe'
$script:nmExe = Join-Path $GccRoot 'bin\riscv32-esp-elf-nm.exe'
foreach ($path in @($definesPath,$cFlagsPath,$includesPath,$configPath,$archivePath,$gccExe,$arExe,$script:nmExe)) { Require-File $path }
$defines = Get-Content -LiteralPath $definesPath -Raw
foreach ($requirement in @(
  @{Name='MAJOR'; Value='2'}, @{Name='MINOR'; Value='6'}, @{Name='PATCH'; Value='0'}
)) {
  if ($defines -notmatch ('(?:^|\s)-DESP_CAM_SENSOR_VER_' + $requirement.Name + '=' + $requirement.Value + '(?:\s|$)')) {
    throw 'SDK esp_cam_sensor version must be exactly 2.6.0; no archive was changed.'
  }
}
$config = Get-Content -LiteralPath $configPath -Raw
foreach ($name in @('CONFIG_IDF_TARGET_ESP32S31','CONFIG_SPIRAM','CONFIG_ESP_VIDEO_ENABLE_DVP_VIDEO_DEVICE','CONFIG_CAMERA_SENSOR_MOTOR_DETECT_METHOD_STATIC_STORE')) {
  if ($config -notmatch ('(?m)^\s*#define\s+' + $name + '\s+1\s*$')) { throw "Required S31 SDK option missing: $name" }
}
if ($config -match '(?m)^\s*#define\s+CONFIG_CAMERA_SENSOR_MOTOR_DETECT_METHOD_DYNAMIC_LINK\s+1\s*$') {
  throw 'Dynamic sensor detection is unsupported by this adapter.'
}
$compilerMachine = ("$(Run-Native $gccExe @('-dumpmachine'))").Trim()
$compilerVersion = ("$(Run-Native $gccExe @('-dumpfullversion'))").Trim()
if ($compilerMachine -ne 'riscv32-esp-elf' -or $compilerVersion -ne '15.2.0') {
  throw "Expected official RISC-V GCC 15.2.0, got $compilerMachine / $compilerVersion."
}

$markerPath = Join-Path $SdkRoot 'lib\nice-gfx-ov3640-adapter.json'
if (Test-Path -LiteralPath $markerPath) {
  $marker = Get-Content -LiteralPath $markerPath -Raw | ConvertFrom-Json
  if ($marker.sourceCommit -eq $provenance.sourceCommit -and (File-Sha $archivePath) -eq $marker.patchedArchiveSha256) {
    Require-File $marker.backupPath
    if ((File-Sha $marker.backupPath) -ne $marker.originalArchiveSha256) { throw 'Original archive backup checksum mismatch.' }
    Write-Output 'OV3640 adapter already installed and verified; no files changed.'
    Write-Output "Archive: $archivePath"
    Write-Output "Backup: $($marker.backupPath)"
    exit 0
  }
  throw 'An adapter marker exists but its source/archive does not match. Restore the recorded backup or use a clean SDK.'
}

$originalHash = File-Sha $archivePath
$originalMembers = @(Run-Native $arExe @('t', $archivePath))
if (@($originalMembers | Where-Object { "$_".Trim() -eq 'esp_cam_sensor_detect.c.obj' }).Count -ne 1) {
  throw 'SDK archive must contain exactly one esp_cam_sensor_detect.c.obj member.'
}
$originalSymbols = Defined-Symbols $archivePath
if ($originalSymbols.ContainsKey('ov3640_detect')) {
  throw 'This SDK already contains OV3640 without this adapter marker; refusing to overwrite it.'
}
if (!$originalSymbols.ContainsKey('esp_cam_sensor_detect_get_array')) { throw 'SDK static detector symbol is missing.' }

$buildRoot = Join-Path ([IO.Path]::GetTempPath()) ('nice-gfx-ov3640-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $buildRoot | Out-Null
$compileArguments = @(
  ('@' + $definesPath), ('@' + $cFlagsPath),
  '-iprefix', ((Join-Path $SdkRoot 'include') + '\'),
  ('@' + $includesPath),
  '-I', (Join-Path $SdkRoot 'qio_opi\include'),
  '-I', (Join-Path $packageRoot 'include'),
  '-I', (Join-Path $packageRoot 'private_include'),
  '-isystem', (Join-Path $GccRoot 'riscv32-esp-elf\include'),
  '-isystem', (Join-Path $GccRoot 'lib\gcc\riscv32-esp-elf\15.2.0\include'),
  '-isystem', (Join-Path $GccRoot 'lib\gcc\riscv32-esp-elf\15.2.0\include-fixed'),
  '-include', (Join-Path $packageRoot 'ov3640_config.h'), '-Os', '-c'
)
$driverObject = Join-Path $buildRoot 'ov3640.c.obj'
$detectorObject = Join-Path $buildRoot 'esp_cam_sensor_detect.c.obj'
Run-Native $gccExe ($compileArguments + @((Join-Path $packageRoot 'src\ov3640.c'),'-o',$driverObject)) | Write-Output
Run-Native $gccExe ($compileArguments + @((Join-Path $packageRoot 'src\esp_cam_sensor_detect.c'),'-o',$detectorObject)) | Write-Output

$driverSymbols = Defined-Symbols $driverObject
if (!$driverSymbols.ContainsKey('ov3640_detect') -or
    !$driverSymbols.ContainsKey('__esp_cam_sensor_detect_fn_ov3640_detect_ESP_CAM_SENSOR_DVP')) {
  throw 'Compiled OV3640 detection entry is missing.'
}
$newDetectorReferences = @{}
foreach ($line in @(Run-Native $script:nmExe @('-u',$detectorObject))) {
  if ("$line" -match '\bU\s+(__esp_cam_sensor_detect_fn_\S+)\s*$') {
    $symbol = $Matches[1]
    $newDetectorReferences[$symbol] = $true
    if (!$originalSymbols.ContainsKey($symbol) -and !$driverSymbols.ContainsKey($symbol)) {
      throw "New detection table refers to an unavailable sensor: $symbol"
    }
  }
}
$originalDetectorDirectory = Join-Path $buildRoot 'original-detector'
New-Item -ItemType Directory -Path $originalDetectorDirectory | Out-Null
Run-Native $arExe @('x',('--output=' + $originalDetectorDirectory),$archivePath,'esp_cam_sensor_detect.c.obj') | Write-Output
foreach ($line in @(Run-Native $script:nmExe @('-u',(Join-Path $originalDetectorDirectory 'esp_cam_sensor_detect.c.obj')))) {
  if ("$line" -match '\bU\s+(__esp_cam_sensor_detect_fn_\S+)\s*$') {
    if (!$newDetectorReferences.ContainsKey($Matches[1])) {
      throw "Existing static detector registration would be lost: $($Matches[1])"
    }
  }
}
$candidatePath = Join-Path $buildRoot 'libespressif__esp_cam_sensor.a'
Copy-Item -LiteralPath $archivePath -Destination $candidatePath
Run-Native $arExe @('rcs',$candidatePath,$detectorObject,$driverObject) | Write-Output
$candidateMembers = @(Run-Native $arExe @('t',$candidatePath))
foreach ($member in $originalMembers) {
  if (@($candidateMembers | Where-Object { "$_".Trim() -eq "$member".Trim() }).Count -ne 1) {
    throw "Archive member was lost or duplicated: $member"
  }
}
if (@($candidateMembers | Where-Object { "$_".Trim() -eq 'ov3640.c.obj' }).Count -ne 1) {
  throw 'Patched archive must contain exactly one OV3640 object.'
}
$candidateSymbols = Defined-Symbols $candidatePath
foreach ($name in $originalSymbols.Keys) {
  if ($name.StartsWith('__esp_cam_sensor_detect_fn_') -and !$candidateSymbols.ContainsKey($name)) {
    throw "Existing sensor detection entry was lost: $name"
  }
}
if (!$candidateSymbols.ContainsKey('ov3640_detect')) { throw 'OV3640 is missing from candidate archive.' }
$patchedHash = File-Sha $candidatePath
if ((File-Sha $archivePath) -ne $originalHash) { throw 'SDK archive changed while compiling; refusing to overwrite concurrent work.' }

$libraryDirectory = Split-Path $archivePath -Parent
$operationId = [guid]::NewGuid().ToString('N')
$backupPath = Join-Path $libraryDirectory ('libespressif__esp_cam_sensor.a.before-ov3640-2.6.0-' + $operationId + '.bak')
$stagePath = Join-Path $libraryDirectory ('libespressif__esp_cam_sensor.a.ov3640-stage-' + $operationId)
Copy-Item -LiteralPath $candidatePath -Destination $stagePath
[IO.File]::Replace($stagePath,$archivePath,$backupPath)
if ((File-Sha $archivePath) -ne $patchedHash -or (File-Sha $backupPath) -ne $originalHash) {
  throw "Archive replacement checksum failed. Original backup: $backupPath"
}
$marker = [ordered]@{
  adapter='nice_gfx optional OV3640 adapter'; componentVersion='2.6.0'
  sourceCommit=$provenance.sourceCommit; compilerMachine=$compilerMachine; compilerVersion=$compilerVersion
  sdkRoot=$SdkRoot; backupPath=$backupPath
  originalArchiveSha256=$originalHash; patchedArchiveSha256=$patchedHash
  sensorMode='OV3640 24MHz DVP VGA UYVY 30fps'; completedAtUtc=[DateTime]::UtcNow.ToString('o')
}
$marker | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $markerPath -Encoding UTF8
Write-Output 'OV3640 adapter installed; existing enabled sensor entries are retained.'
Write-Output "Archive: $archivePath"
Write-Output "Original backup: $backupPath"
Write-Output "Archive SHA256: $patchedHash"
Write-Output "Build files: $buildRoot"
Write-Output 'Rebuild the Arduino sketch with a fresh build directory so it relinks this archive.'

