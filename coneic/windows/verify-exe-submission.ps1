# Fail closed before publishing an EXE/MSI Store submission. Never signs or uploads.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Installer,
    [Parameter(Mandatory=$true)][string]$Payload,
    [Parameter(Mandatory=$true)][string]$Report,
    [string]$SignTool,
    [switch]$OfflineSilentInstallationTestPassed
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$installerPath=(Resolve-Path -LiteralPath $Installer).Path
$payloadRoot=(Resolve-Path -LiteralPath $Payload).Path
if([IO.Path]::GetExtension($installerPath) -notin @('.exe','.msi')){throw 'Installer must be EXE or MSI.'}
if(-not $SignTool){
    $sdk=Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
    foreach($directory in @(Get-ChildItem -LiteralPath $sdk -Directory | Where-Object {$_.Name -match '^10\.0\.\d+\.\d+$'} | Sort-Object {[version]$_.Name} -Descending)){
        $candidate=Join-Path $directory.FullName 'x64\signtool.exe'
        if(Test-Path -LiteralPath $candidate){$SignTool=$candidate;break}
    }
}
if(-not $SignTool){throw 'Windows SDK SignTool is required.'}
$checks=[Collections.Generic.List[object]]::new()
$launcher=Join-Path $payloadRoot 'Coneic.exe'
$launcherHash=if(Test-Path -LiteralPath $launcher){(Get-FileHash -LiteralPath $launcher -Algorithm SHA256).Hash}else{''}
$installerHash=(Get-FileHash -LiteralPath $installerPath -Algorithm SHA256).Hash
$checks.Add([pscustomobject]@{Name='Submission is not the standalone player';Passed=($launcherHash -ne '' -and $installerHash -ne $launcherHash)})
$plugins=@(Get-ChildItem -LiteralPath (Join-Path $payloadRoot 'plugins') -Filter '*.dll' -Recurse -File -ErrorAction SilentlyContinue)
$checks.Add([pscustomobject]@{Name='Complete application payload';Passed=($plugins.Count -ge 360 -and (Test-Path -LiteralPath (Join-Path $payloadRoot 'libvlc.dll')) -and (Test-Path -LiteralPath (Join-Path $payloadRoot 'libvlccore.dll')))})
$checks.Add([pscustomobject]@{Name='Offline silent installation and uninstall tested';Passed=[bool]$OfflineSilentInstallationTestPassed})
$files=[Collections.Generic.List[string]]::new()
$files.Add($installerPath)
# Inspect the MZ header, rather than trusting filename extensions to identify PE files.
foreach($file in Get-ChildItem -LiteralPath $payloadRoot -File -Recurse){
    $stream=[IO.File]::OpenRead($file.FullName)
    try{$isPE=$stream.ReadByte() -eq 0x4D -and $stream.ReadByte() -eq 0x5A}finally{$stream.Dispose()}
    if($isPE -and $file.FullName -ne $installerPath){$files.Add($file.FullName)}
}
$signatures=@(foreach($file in $files){
    $signature=Get-AuthenticodeSignature -LiteralPath $file
    $trusted=$signature.Status -eq 'Valid'
    $verified=$false
    $strongDigest=$false
    if($trusted){
        $verification=(& $SignTool verify /pa /all /v $file 2>&1 | Out-String)
        $verified=$LASTEXITCODE -eq 0
        # Fail closed if the SDK does not report a recognized strong file digest.
        # This checks the file digest, NOT the certificate's signature algorithm.
        $strongDigest=$verification -match '(?i)Hash of file \(sha(256|384|512)\)'
    }
    [pscustomobject]@{Path=$file;Status=$signature.Status.ToString();Trusted=$trusted;SignToolVerified=$verified;SHA256OrStronger=$strongDigest;Passed=($trusted -and $verified -and $strongDigest)}
})
$failed=@($signatures | Where-Object {-not $_.Passed})
$passed=@($checks | Where-Object {-not $_.Passed}).Count -eq 0 -and $failed.Count -eq 0
$result=[pscustomobject]@{Passed=$passed;InstallerSHA256=$installerHash;Checks=$checks;PEAndInstallerFiles=$signatures.Count;SignatureFailures=$failed.Count;Signatures=$signatures;Limits='This gate is not Microsoft certification. Installer behavior must be tested separately in a clean Windows VM. Preserve the submitted URL and use a new versioned URL for replacement bytes.'}
$reportPath=[IO.Path]::GetFullPath($Report)
[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($reportPath)) | Out-Null
$result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $reportPath -Encoding utf8
[pscustomobject]@{Passed=$passed;PEAndInstallerFiles=$signatures.Count;SignatureFailures=$failed.Count;Report=$reportPath} | ConvertTo-Json
if(-not $passed){exit 1}
