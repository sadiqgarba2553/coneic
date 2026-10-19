[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Installer,[Parameter(Mandatory=$true)][string]$Workspace)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$workspacePath=[IO.Path]::GetFullPath($Workspace).TrimEnd('\')
$target=Join-Path $workspacePath ('coneic-installer-test-'+[guid]::NewGuid().ToString('N'))
if(Test-Path 'HKCU:\Software\BlueCloud\Coneic'){throw 'A standalone Coneic installation is present; use a clean Windows test profile for the installer lifecycle test.'}
$priorOwner=$null
if(Test-Path 'HKCU:\Software\Clients\Media\Coneic'){$priorOwner=(Get-ItemProperty 'HKCU:\Software\Clients\Media\Coneic').InstallPath}
$shortcutRoot=Join-Path ([Environment]::GetFolderPath('Programs')) 'Coneic'
$shortcuts=@{}
foreach($name in @('Coneic Player.lnk','Set Coneic as default.lnk')){
    $file=Join-Path $shortcutRoot $name
    if(Test-Path -LiteralPath $file){$shortcuts[$name]=[IO.File]::ReadAllBytes($file)}
}
$passed=$false
try{
    New-Item -ItemType Directory -Path $target | Out-Null
    $process=Start-Process -FilePath (Resolve-Path -LiteralPath $Installer).Path -ArgumentList @('/S',"/D=$target") -WindowStyle Hidden -Wait -PassThru
    if($process.ExitCode -ne 0){throw 'Silent Coneic installation failed.'}
    $product=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'product.json') -Raw | ConvertFrom-Json
    $exe=Join-Path $target 'Coneic.exe'
    if([Diagnostics.FileVersionInfo]::GetVersionInfo($exe).ProductVersion -ne $product.displayVersion){throw 'Installed version differs from the release.'}
    $registration=(Get-ItemProperty 'HKCU:\Software\Clients\Media\Coneic').InstallPath
    if($registration -ne $target){throw 'Installer did not register the installed location.'}
    if(-not (Test-Path -LiteralPath (Join-Path $shortcutRoot 'Coneic Player.lnk'))){throw 'Start shortcut is missing.'}
    if(@(Get-ChildItem -LiteralPath (Join-Path $target 'plugins') -Recurse -Filter *.dll).Count -lt 360){throw 'Installer omitted media plugins.'}
    $marker=Join-Path $target 'user-created-file.txt'
    [IO.File]::WriteAllText($marker,'retain this file')
    $uninstaller=Join-Path $target 'Uninstall Coneic.exe'
    Start-Process -FilePath $uninstaller -ArgumentList '/S' -WindowStyle Hidden -Wait | Out-Null
    $deadline=[DateTime]::UtcNow.AddSeconds(45)
    while(((Test-Path -LiteralPath $exe) -or (Test-Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\BlueCloud.Coneic')) -and [DateTime]::UtcNow -lt $deadline){Start-Sleep -Milliseconds 250}
    if(Test-Path -LiteralPath $exe){throw 'Uninstaller did not remove the player.'}
    if([IO.File]::ReadAllText($marker) -ne 'retain this file'){throw 'Uninstaller removed user-created files.'}
    if(Test-Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\BlueCloud.Coneic'){throw 'Uninstall registration remains.'}
    $passed=$true
    Write-Output "PASS: Coneic $($product.displayVersion) installs silently, registers Windows integration and uninstalls while preserving extra user files."
}finally{
    if(-not $passed -and (Test-Path -LiteralPath (Join-Path $target 'Uninstall Coneic.exe'))){Start-Process -FilePath (Join-Path $target 'Uninstall Coneic.exe') -ArgumentList '/S' -WindowStyle Hidden -Wait | Out-Null}
    if($priorOwner){
        $helper=Join-Path $priorOwner 'Coneic-shell\register-file-associations.ps1'
        if(Test-Path -LiteralPath $helper){& $helper -AppDirectory $priorOwner | Out-Null}
    }
    if($shortcuts.Count){New-Item -ItemType Directory -Path $shortcutRoot -Force | Out-Null;foreach($name in $shortcuts.Keys){[IO.File]::WriteAllBytes((Join-Path $shortcutRoot $name),$shortcuts[$name])}}
    # The only recursive removal is the checked generated test directory.
    $resolved=[IO.Path]::GetFullPath($target)
    if(-not $resolved.StartsWith($workspacePath+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Unsafe test cleanup target.'}
    if(Test-Path -LiteralPath $resolved){Remove-Item -LiteralPath $resolved -Recurse -Force}
}
