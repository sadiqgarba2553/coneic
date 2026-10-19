[CmdletBinding()]
param([Parameter(Mandatory=$true)][string[]]$Files)
$ErrorActionPreference='Stop'
if($env:BLUECLOUD_SIGN_WINDOWS -ne '1'){return}
if($env:BLUECLOUD_CERT_THUMBPRINT -notmatch '^[a-fA-F0-9]{40}$'){throw 'Signing requires BLUECLOUD_CERT_THUMBPRINT for a private certificate in the Windows certificate store.'}
$signTool=if($env:CONEIC_SIGNTOOL){$env:CONEIC_SIGNTOOL}else{(Get-Command signtool.exe -ErrorAction Stop).Source}
$timestamp=if($env:BLUECLOUD_TIMESTAMP_URL){$env:BLUECLOUD_TIMESTAMP_URL}else{'http://timestamp.digicert.com'}
foreach($file in $Files){
    $target=(Resolve-Path -LiteralPath $file).Path
    & $signTool sign /sha1 $env:BLUECLOUD_CERT_THUMBPRINT /fd SHA256 /tr $timestamp /td SHA256 $target
    if($LASTEXITCODE -ne 0){throw 'Authenticode signing failed; do not publish this build.'}
    & $signTool verify /pa $target
    if($LASTEXITCODE -ne 0){throw 'The new signature could not be verified.'}
}
