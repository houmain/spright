
$url64 = "https://github.com/houmain/spright/releases/download/3.5.0/spright-3.5.0-win64.zip"
$checksum64 = "5909644681DE388A6DCD7F7623CE6AB154D2B86906AA13B2085050309CB0A00E"

Install-ChocolateyZipPackage -PackageName "spright" -UnzipLocation "$(Split-Path -Parent $MyInvocation.MyCommand.Definition)" -Url64bit "$url64" -ChecksumType "sha256" -Checksum64 $checksum64
