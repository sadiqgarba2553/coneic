# Build an unsigned Store MSIX, or an explicitly marked local validation package.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$App,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [ValidateSet('Store','Test')][string]$Mode='Store',
    [string]$IdentityFile,
    [string]$Python='python',
    [string]$MakeAppx
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$appRoot=(Resolve-Path -LiteralPath $App).Path
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Use a new output directory; builds never overwrite earlier artifacts.'}
if($out.StartsWith($appRoot.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)){
    throw 'The output directory must be outside the input application.'
}
$product=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'product.json') -Raw | ConvertFrom-Json
$v=[version]$product.storeVersion
if($v.Major -lt 1 -or $v.Revision -ne 0 -or @($v.Major,$v.Minor,$v.Build,$v.Revision | Where-Object {$_ -gt 65535}).Count){throw 'Invalid Microsoft Store version.'}
if($Mode -eq 'Store'){
    if(-not $IdentityFile){throw 'Store mode requires the exact Partner Center identity JSON. Use -Mode Test for local validation.'}
    $identity=Get-Content -LiteralPath $IdentityFile -Raw | ConvertFrom-Json
    foreach($field in @('Name','Publisher','PublisherDisplayName')){
        if(-not $identity.$field -or $identity.$field -match 'REPLACE|PLACEHOLDER|LocalValidation'){
            throw "Missing real Partner Center identity field: $field"
        }
    }
    if($identity.Name -notmatch '^[A-Za-z0-9.-]{3,50}$' -or $identity.Publisher -notmatch '^CN='){
        throw 'Invalid package identity format. Copy the exact values from Partner Center.'
    }
}else{
    if($IdentityFile){throw 'Test mode uses its own validation identity; omit IdentityFile.'}
    $identity=[pscustomobject]@{Name='BlueCloud.Coneic.LocalValidation';Publisher='CN=Coneic Local Validation';PublisherDisplayName='BLUECLOUD (local validation)'}
}
if(-not $MakeAppx){
    $sdk=Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
    $candidates=@(Get-ChildItem -LiteralPath $sdk -Directory | Where-Object {$_.Name -match '^10\.0\.\d+\.\d+$'} | Sort-Object {[version]$_.Name} -Descending)
    foreach($candidate in $candidates){
        $tool=Join-Path $candidate.FullName 'x64\makeappx.exe'
        if(Test-Path -LiteralPath $tool){$MakeAppx=$tool;break}
    }
}
if(-not $MakeAppx -or -not (Test-Path -LiteralPath $MakeAppx)){throw 'Install the Windows SDK MSIX packaging tools, or provide -MakeAppx.'}
New-Item -ItemType Directory -Path $out | Out-Null
$layout=Join-Path $out 'layout'
New-Item -ItemType Directory -Path $layout | Out-Null
$payload=Join-Path $layout 'Coneic'
& $Python (Join-Path $PSScriptRoot 'prepare_payload.py') --app $appRoot --stage $payload --report (Join-Path $out 'payload-audit.json')
if($LASTEXITCODE -ne 0){throw 'Full application preparation failed.'}
foreach($name in @('Coneic.exe','vlc.exe')){
    $exe=Join-Path $payload $name
    if(Test-Path -LiteralPath $exe){
        $info=[Diagnostics.FileVersionInfo]::GetVersionInfo($exe)
        if($info.FileVersion -ne $product.fileVersion -or $info.ProductVersion -ne $product.displayVersion -or $info.CompanyName -ne $product.company){throw "Incorrect launcher metadata: $name"}
    }
}
$assets=Join-Path $layout 'Assets'
New-Item -ItemType Directory -Path $assets | Out-Null
Add-Type -AssemblyName System.Drawing
$image=[Drawing.Image]::FromFile((Join-Path $PSScriptRoot '..\..\share\coneic\coneic-512.png'))
try{
    foreach($size in @(44,50,150)){
        $bitmap=New-Object Drawing.Bitmap($size,$size)
        $graphics=[Drawing.Graphics]::FromImage($bitmap)
        try{
            $graphics.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $graphics.DrawImage($image,0,0,$size,$size)
            $bitmap.Save((Join-Path $assets "Logo$size.png"),[Drawing.Imaging.ImageFormat]::Png)
        }finally{$graphics.Dispose();$bitmap.Dispose()}
    }
}finally{$image.Dispose()}
function XmlEscape([string]$Text){[Security.SecurityElement]::Escape($Text)}
$name=XmlEscape $identity.Name
$publisher=XmlEscape $identity.Publisher
$publisherDisplay=XmlEscape $identity.PublisherDisplayName
$display='Coneic'
if($Mode -eq 'Test'){$display='Coneic Alpha Validation'}
$formats=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'file-types.json') -Raw | ConvertFrom-Json
$fileAssociations=($formats | ForEach-Object {
    $stem=$_.extension.Substring(1)
    $associationName='coneic'+$stem.Replace('-','')
    $label=XmlEscape ($stem.ToUpperInvariant()+' '+$_.kind+' file (Coneic)')
    '<uap:Extension Category="windows.fileTypeAssociation"><uap3:FileTypeAssociation Name="'+$associationName+'" Parameters="--started-from-file &quot;%1&quot;">'+
    '<uap:DisplayName>'+$label+'</uap:DisplayName><uap:Logo>Coneic\Coneic-shell\icons\'+$stem+'.png</uap:Logo>'+
    '<uap:SupportedFileTypes><uap:FileType>'+$_.extension+'</uap:FileType></uap:SupportedFileTypes></uap3:FileTypeAssociation></uap:Extension>'
}) -join "`n"
$manifest=@"
<?xml version="1.0" encoding="utf-8"?>
<Package xmlns="http://schemas.microsoft.com/appx/manifest/foundation/windows10"
 xmlns:uap="http://schemas.microsoft.com/appx/manifest/uap/windows10"
 xmlns:uap3="http://schemas.microsoft.com/appx/manifest/uap/windows10/3"
 xmlns:rescap="http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities"
 IgnorableNamespaces="uap uap3 rescap">
 <Identity Name="$name" Publisher="$publisher" Version="$($product.storeVersion)" ProcessorArchitecture="x64" />
 <Properties><DisplayName>$display</DisplayName><PublisherDisplayName>$publisherDisplay</PublisherDisplayName>
 <Description>Coneic $($product.displayVersion), a media player by BLUECLOUD, based on VLC.</Description><Logo>Assets\Logo50.png</Logo></Properties>
 <Resources><Resource Language="en-US" /></Resources>
 <Dependencies><TargetDeviceFamily Name="Windows.Desktop" MinVersion="10.0.17763.0" MaxVersionTested="10.0.26100.0" /></Dependencies>
 <Applications><Application Id="Coneic" Executable="Coneic\Coneic.exe" EntryPoint="Windows.FullTrustApplication">
 <uap:VisualElements DisplayName="$display" Description="Coneic media player by BLUECLOUD" BackgroundColor="transparent"
 Square150x150Logo="Assets\Logo150.png" Square44x44Logo="Assets\Logo44.png" />
 <Extensions>$fileAssociations</Extensions>
 </Application></Applications>
 <Capabilities><rescap:Capability Name="runFullTrust" /></Capabilities>
</Package>
"@
$manifestPath=Join-Path $layout 'AppxManifest.xml'
[IO.File]::WriteAllText($manifestPath,$manifest,(New-Object Text.UTF8Encoding($false)))
$suffix='Store'
if($Mode -eq 'Test'){$suffix='TEST-IDENTITY-NOT-FOR-SUBMISSION'}
$package=Join-Path $out "Coneic-$($product.storeVersion)-x64-$suffix.msix"
# Keep validation enabled: never use /nv to conceal a schema or payload error.
& $MakeAppx pack /d $layout /p $package /h SHA256 2>&1 | Out-File -LiteralPath (Join-Path $out 'makeappx.log') -Encoding utf8
if($LASTEXITCODE -ne 0){throw 'MakeAppx validation/packaging failed.'}
& $Python (Join-Path $PSScriptRoot 'verify_msix.py') --package $package --layout $layout --mode $Mode --output (Join-Path $out 'msix-audit.json')
if($LASTEXITCODE -ne 0){throw 'MSIX archive verification failed.'}
[pscustomobject]@{Mode=$Mode;Package=$package;SHA256=(Get-FileHash -LiteralPath $package -Algorithm SHA256).Hash;Identity=$identity;StoreVersion=$product.storeVersion;ProductVersion=$product.displayVersion;Signed=$false;ReadyToSubmit=$false;Remaining='Real Partner Center identity and packaged-app acceptance checks; Microsoft performs Store signing. No submission is performed by this script.'} |
 ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $out 'build-result.json') -Encoding utf8
Get-Content -LiteralPath (Join-Path $out 'build-result.json')
