# Per-user registration; Windows owns the final default-app choice.
[CmdletBinding()]
param(
    [ValidateSet('Register','Unregister')][string]$Action='Register',
    [string]$AppDirectory=(Split-Path -Parent $PSScriptRoot),
    [switch]$OpenSettings,
    # Tests use a private hive subtree, never real file associations.
    [string]$TestId
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$prefix='Software'
if($TestId){
    if($TestId -notmatch '^[a-f0-9]{32}$'){throw 'TestId must be a GUID in N format.'}
    $prefix="Software\BlueCloud\ConeicAssociationTests\$TestId"
    if($OpenSettings){throw 'Tests cannot open default-app settings.'}
}
$appPath=[IO.Path]::GetFullPath($AppDirectory).TrimEnd('\')
$exe=Join-Path $appPath 'Coneic.exe'
# Packaged apps are registered by their manifest, rather than desktop registry keys.
$packageManifest=Join-Path (Split-Path -Parent $appPath) 'AppxManifest.xml'
if(-not $TestId -and (Test-Path -LiteralPath $packageManifest)){
    if($OpenSettings){
        $package=Get-AppxPackage | Where-Object {$_.InstallLocation -eq (Split-Path -Parent $appPath)} | Select-Object -First 1
        if(-not $package){throw 'The Coneic package is not installed.'}
        Start-Process ('ms-settings:defaultapps?registeredAUMID='+[Uri]::EscapeDataString($package.PackageFamilyName+'!Coneic')) -WindowStyle Hidden
    }
    Write-Output 'Packaged Coneic file associations are managed by Windows.'
    return
}
$formats=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'file-types.json') -Raw | ConvertFrom-Json
$hive=[Microsoft.Win32.Registry]::CurrentUser
$client="$prefix\Clients\Media\Coneic"
$classes="$prefix\Classes"
function Write-Value([string]$Key,[string]$Name,[string]$Value){
    $handle=$hive.CreateSubKey($Key)
    try{$handle.SetValue($Name,$Value,[Microsoft.Win32.RegistryValueKind]::String)}finally{$handle.Dispose()}
}
function Read-Value([string]$Key,[string]$Name){
    $handle=$hive.OpenSubKey($Key)
    if($handle){try{return $handle.GetValue($Name,$null)}finally{$handle.Dispose()}}
}
function Remove-Value([string]$Key,[string]$Name){
    $handle=$hive.OpenSubKey($Key,$true)
    if($handle){try{$handle.DeleteValue($Name,$false)}finally{$handle.Dispose()}}
}
if($Action -eq 'Register'){
    if(-not (Test-Path -LiteralPath $exe -PathType Leaf)){throw 'Coneic.exe is missing.'}
    # Validate every icon before writing any registration.
    foreach($format in $formats){
        if($format.extension -notmatch '^\.[a-z0-9-]+$' -or $format.kind -notin @('audio','video','playlist')){throw 'Invalid file-type catalog.'}
        $icon=Join-Path $PSScriptRoot ('icons\'+$format.extension.Substring(1)+'.ico')
        if(-not (Test-Path -LiteralPath $icon -PathType Leaf)){throw "Missing format icon: $icon"}
    }
    $command='"'+$exe+'" --started-from-file "%1"'
    Write-Value $client 'InstallPath' $appPath
    Write-Value "$client\Capabilities" 'ApplicationName' 'Coneic Player'
    Write-Value "$client\Capabilities" 'ApplicationDescription' 'Play audio, video and playlists with Coneic by BLUECLOUD.'
    Write-Value "$client\Capabilities" 'ApplicationIcon' ('"'+$exe+'",0')
    Write-Value "$prefix\RegisteredApplications" 'Coneic' "$client\Capabilities"
    Write-Value "$classes\Applications\Coneic.exe" 'FriendlyAppName' 'Coneic Player'
    Write-Value "$classes\Applications\Coneic.exe" 'InstallPath' $appPath
    Write-Value "$classes\Applications\Coneic.exe\shell\open\command" '' $command
    foreach($format in $formats){
        $extension=$format.extension
        $progId='Coneic'+$extension
        $label=$extension.Substring(1).ToUpperInvariant()+' '+$format.kind+' file (Coneic)'
        $icon=Join-Path $PSScriptRoot ('icons\'+$extension.Substring(1)+'.ico')
        Write-Value "$classes\$progId" '' $label
        Write-Value "$classes\$progId" 'FriendlyTypeName' $label
        Write-Value "$classes\$progId" 'InstallPath' $appPath
        Write-Value "$classes\$progId" 'MultiSelectModel' 'Player'
        Write-Value "$classes\$progId\DefaultIcon" '' ('"'+$icon+'",0')
        Write-Value "$classes\$progId\shell\open" 'FriendlyAppName' 'Coneic Player'
        Write-Value "$classes\$progId\shell\open\command" '' $command
        Write-Value "$classes\$extension\OpenWithProgids" $progId ''
        Write-Value "$classes\Applications\Coneic.exe\SupportedTypes" $extension ''
        Write-Value "$client\Capabilities\FileAssociations" $extension $progId
    }
}else{
    # An older portable copy must never unregister a newer installation.
    if((Read-Value $client 'InstallPath') -ne $appPath){Write-Output 'This copy does not own the current Coneic registration.';return}
    foreach($format in $formats){
        $progId='Coneic'+$format.extension
        if((Read-Value "$classes\$progId" 'InstallPath') -eq $appPath){
            Remove-Value "$classes\$($format.extension)\OpenWithProgids" $progId
            $hive.DeleteSubKeyTree("$classes\$progId",$false)
        }
    }
    if((Read-Value "$classes\Applications\Coneic.exe" 'InstallPath') -eq $appPath){$hive.DeleteSubKeyTree("$classes\Applications\Coneic.exe",$false)}
    Remove-Value "$prefix\RegisteredApplications" 'Coneic'
    $hive.DeleteSubKeyTree($client,$false)
}
if(-not $TestId){
    if(-not ('ConeicShell.Notification' -as [type])){
        Add-Type -TypeDefinition 'namespace ConeicShell { public static class Notification { [System.Runtime.InteropServices.DllImport("shell32.dll")] public static extern void SHChangeNotify(uint e, uint f, System.IntPtr a, System.IntPtr b); } }'
    }
    [ConeicShell.Notification]::SHChangeNotify(0x08000000,0,[IntPtr]::Zero,[IntPtr]::Zero)
    if($OpenSettings -and $Action -eq 'Register'){Start-Process 'ms-settings:defaultapps?registeredAppUser=Coneic' -WindowStyle Hidden}
}
Write-Output "$Action completed for $($formats.Count) Coneic media formats."
