[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$App,[Parameter(Mandatory=$true)][string]$OutputDirectory,[string]$MakeNsis)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$appRoot=(Resolve-Path -LiteralPath $App).Path
$output=[IO.Path]::GetFullPath($OutputDirectory)
if($output.StartsWith($appRoot+'\',[StringComparison]::OrdinalIgnoreCase) -or $output -eq $appRoot){throw 'Output must be outside the app payload.'}
$product=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'product.json') -Raw | ConvertFrom-Json
foreach($name in @('Coneic.exe','Coneic.ico','COPYING.txt','Coneic-shell\register-file-associations.ps1')){
    if(-not (Test-Path -LiteralPath (Join-Path $appRoot $name) -PathType Leaf)){throw "Missing payload file: $name"}
}
$info=[Diagnostics.FileVersionInfo]::GetVersionInfo((Join-Path $appRoot 'Coneic.exe'))
if($info.ProductVersion -ne $product.displayVersion){throw 'The Coneic executable version does not match product.json.'}
if(-not $MakeNsis){
    $cache=Join-Path $env:LOCALAPPDATA 'electron-builder\Cache\nsis'
    $tool=Get-ChildItem -LiteralPath $cache -Recurse -Filter makensis.exe | Where-Object {$_.Directory.Name -eq 'Bin'} | Select-Object -First 1
    if(-not $tool){throw 'NSIS is missing. Build the launcher first or provide -MakeNsis.'}
    $MakeNsis=$tool.FullName
}
New-Item -ItemType Directory -Path $output -Force | Out-Null
$deleteInclude=Join-Path $output 'coneic-uninstall-files.nsh'
$files=@(Get-ChildItem -LiteralPath $appRoot -Recurse -File)
$lines=[Collections.Generic.List[string]]::new()
foreach($file in $files){
    $relative=$file.FullName.Substring($appRoot.Length+1)
    if($relative.Contains('$') -or $relative.Contains('"')){throw 'Unsupported character in payload filename.'}
    $lines.Add('Delete "$INSTDIR\'+$relative+'"')
}
foreach($directory in (Get-ChildItem -LiteralPath $appRoot -Recurse -Directory | Sort-Object {$_.FullName.Length} -Descending)){
    $lines.Add('RMDir "$INSTDIR\'+$directory.FullName.Substring($appRoot.Length+1)+'"')
}
[IO.File]::WriteAllLines($deleteInclude,$lines,[Text.UTF8Encoding]::new($true))
$installer=Join-Path $output "Coneic-Setup-$($product.displayVersion)-x64.exe"
if(Test-Path -LiteralPath $installer){throw 'Installer already exists; retain immutable build outputs.'}
$nsisRoot=Split-Path -Parent $MakeNsis
if(-not (Test-Path -LiteralPath (Join-Path $nsisRoot 'Include'))){$nsisRoot=Split-Path -Parent $nsisRoot}
$previousNsisDir=$env:NSISDIR
try{
    $env:NSISDIR=$nsisRoot
    & $MakeNsis '/V2' "/DAPPDIR=$appRoot" "/DOUTFILE=$installer" "/DPRODUCT_VERSION=$($product.displayVersion)" "/DFILE_VERSION=$($product.fileVersion)" "/DDELETE_INCLUDE=$deleteInclude" (Join-Path $PSScriptRoot 'installer.nsi')
    if($LASTEXITCODE -ne 0){throw 'Coneic installer compilation failed.'}
}finally{$env:NSISDIR=$previousNsisDir}
Get-Item -LiteralPath $installer | Select-Object FullName,Length
