[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$App)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$appPath=(Resolve-Path -LiteralPath $App).Path
$script=Join-Path $appPath 'Coneic-shell\register-file-associations.ps1'
$formats=Get-Content -LiteralPath (Join-Path $appPath 'Coneic-shell\file-types.json') -Raw | ConvertFrom-Json
$id=[guid]::NewGuid().ToString('N')
$root="Software\BlueCloud\ConeicAssociationTests\$id"
$hive=[Microsoft.Win32.Registry]::CurrentUser
$checks=0
function Assert($Condition,[string]$Message){if(-not $Condition){throw $Message};$script:checks++}
function Read([string]$Key,[string]$Name){$k=$hive.OpenSubKey("$root\$Key");if($k){try{return $k.GetValue($Name,$null)}finally{$k.Dispose()}}}
try{
    & $script -TestId $id -AppDirectory $appPath
    & $script -TestId $id -AppDirectory $appPath # Repair is idempotent.
    Assert ((Read 'RegisteredApplications' 'Coneic') -eq "$root\Clients\Media\Coneic\Capabilities") 'Missing Default apps registration'
    $command='"'+(Join-Path $appPath 'Coneic.exe')+'" --started-from-file "%1"'
    $hashes=@{}
    Add-Type -AssemblyName System.Drawing
    foreach($f in $formats){
        $ext=$f.extension;$progId='Coneic'+$ext
        Assert ((Read 'Clients\Media\Coneic\Capabilities\FileAssociations' $ext) -eq $progId) "Missing capability $ext"
        $key=$hive.OpenSubKey("$root\Classes\$ext\OpenWithProgids")
        try{Assert ($key.GetValueNames() -contains $progId) "Missing Open with $ext"}finally{$key.Dispose()}
        Assert ((Read "Classes\$progId\shell\open\command" '') -eq $command) "Unquoted command $ext"
        $icon=Join-Path $appPath ('Coneic-shell\icons\'+$ext.Substring(1)+'.ico')
        Assert ((Read "Classes\$progId\DefaultIcon" '') -eq ('"'+$icon+'",0')) "Wrong icon $ext"
        $image=[Drawing.Icon]::new($icon,256,256)
        try{Assert ($image.Width -ge 128) "Unreadable icon $ext"}finally{$image.Dispose()}
        $hash=(Get-FileHash -LiteralPath $icon -Algorithm SHA256).Hash
        Assert (-not $hashes.ContainsKey($hash)) "Duplicate format artwork $ext"
        $hashes[$hash]=$true
        Assert ($null -eq (Read "Classes\$ext" '')) "Registration changed extension default $ext"
    }
    # Other apps' Open with entries and existing choices survive registration/removal.
    $key=$hive.CreateSubKey("$root\Classes\.wav\OpenWithProgids");$key.SetValue('OtherPlayer.wav','');$key.Dispose()
    $key=$hive.CreateSubKey("$root\Classes\.wav");$key.SetValue('','OtherPlayer.wav');$key.Dispose()
    # A stale installation has no ownership and must do nothing.
    & $script -Action Unregister -TestId $id -AppDirectory ($appPath+'-old')
    Assert ((Read 'Classes\Coneic.wav\shell\open\command' '') -eq $command) 'Old copy removed active registration'
    & $script -Action Unregister -TestId $id -AppDirectory $appPath
    Assert ($null -eq (Read 'RegisteredApplications' 'Coneic')) 'Default apps entry remains'
    foreach($f in $formats){Assert ($null -eq (Read ('Classes\Coneic'+$f.extension) 'InstallPath')) ('ProgID remains: '+$f.extension)}
    Assert ((Read 'Classes\.wav' '') -eq 'OtherPlayer.wav') 'Another default was changed'
    $key=$hive.OpenSubKey("$root\Classes\.wav\OpenWithProgids")
    try{Assert ($key.GetValueNames() -contains 'OtherPlayer.wav') 'Another app was removed'}finally{$key.Dispose()}
    & $script -Action Unregister -TestId $id -AppDirectory $appPath
    Write-Output "PASS: $checks checks across $($formats.Count) formats; private test registry only."
}finally{$hive.DeleteSubKeyTree($root,$false)}
