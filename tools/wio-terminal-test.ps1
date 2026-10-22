param(
  [ValidateSet('Validation','Benchmark')][string]$Mode='Validation',
  [string]$Port='',
  [string]$ArduinoCli='arduino-cli',
  [string]$DataDirectory='',
  [ValidateRange(0,7)][int]$Rotation=3,
  [ValidateRange(1,100)][int]$Rounds=3
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$work=Join-Path $root '.build/wio-terminal'
New-Item -ItemType Directory -Force $work | Out-Null
if(!$DataDirectory){$DataDirectory=Join-Path (Split-Path $root -Parent) '.wio-tools/data'}
if(!(Test-Path -LiteralPath $DataDirectory)){throw 'Pass -DataDirectory with an Arduino CLI data directory containing Seeeduino:samd.'}
$data=(Resolve-Path -LiteralPath $DataDirectory).Path.Replace('\','/')
$local=$work.Replace('\','/')
$config=Join-Path $work 'arduino-cli.yaml'
@"
directories:
  data: $data
  downloads: $local/downloads
  user: $local/user
build_cache:
  path: $local/cache
"@ | Set-Content -LiteralPath $config -Encoding UTF8
$fqbn='Seeeduino:samd:seeed_wio_terminal'
if(!$Port){
  $boards=(& $ArduinoCli board list --config-file $config --format json | ConvertFrom-Json).detected_ports
  $found=@($boards | Where-Object {$_.matching_boards.fqbn -contains $fqbn})
  if($found.Count -ne 1){throw 'Specify -Port when zero or multiple Wio Terminals are connected.'}
  $Port=$found[0].port.address
}
$name=if($Mode -eq 'Validation'){'WioTerminal_Validation'}else{'Graphics_Benchmark'}
$sketch=Join-Path $root "examples/10_Performance/$name"
$build=Join-Path $work ('build-'+$name)
if($Mode -eq 'Benchmark'){
  $staged=Join-Path $work "sketches/$name"
  New-Item -ItemType Directory -Force $staged | Out-Null
  Copy-Item -Path (Join-Path $sketch '*') -Destination $staged -Force
  $app=Join-Path $staged 'BenchmarkApp.h'
  $content=[IO.File]::ReadAllText($app) -replace 'constexpr uint8_t kRotation = \d+;',"constexpr uint8_t kRotation = $Rotation;"
  [IO.File]::WriteAllText($app,$content)
  $sketch=$staged
}
& $ArduinoCli compile --fqbn $fqbn --config-file $config --library $root --build-path $build $sketch
if($LASTEXITCODE){throw 'Compilation failed.'}
& $ArduinoCli upload --fqbn $fqbn --config-file $config --port $Port --input-dir $build
if($LASTEXITCODE){throw 'Upload failed.'}
$log=Join-Path $work ($Mode+'-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'.txt')
$serial=[IO.Ports.SerialPort]::new($Port,115200,[IO.Ports.Parity]::None,8,[IO.Ports.StopBits]::One)
$serial.DtrEnable=$true;$serial.ReadTimeout=250;$serial.ReadBufferSize=131072
$writer=[IO.StreamWriter]::new($log,$false,[Text.UTF8Encoding]::new($false))
$timer=[Diagnostics.Stopwatch]::StartNew();$rows=0;$done=$false;$failed=$false
try{
  $serial.Open();$serial.Write('r')
  while($timer.Elapsed.TotalSeconds -lt (120+90*$Rounds)){
    try{$line=$serial.ReadLine().Trim()}catch [TimeoutException]{continue}
    $writer.WriteLine($line);$writer.Flush();Write-Output $line
    if($line -match '^VALIDATE_DONE checks=\d+ failures=(\d+)'){
      $failed=$Matches[1] -ne '0';$done=$true;break
    }
    if($line -match '^BENCH_(SKIP|FATAL|ABORT)'){$failed=$true}
    if($line -match '^BENCH_CSV,[^,]+,(OK|ERROR|NO_FRAMES),'){
      if($Matches[1] -ne 'OK'){$failed=$true};++$rows
      if($rows -ge 9*$Rounds){$done=$true;break}
      if($rows%9 -eq 0){$serial.Write('r')}
    }
  }
}finally{$writer.Dispose();$serial.Dispose()}
Write-Output "Saved $log"
if(!$done -or $failed){throw 'Hardware run failed or did not finish; inspect the saved transcript.'}
