
$url64 = "https://github.com/houmain/spright/releases/download/3.5.0/spright-3.5.0-win64.zip"
$checksum64 = "B29705265603D61D843E6F58DAB5858A417C060E0D0E7C9446B6FAA3AFC39F9B"

Install-ChocolateyZipPackage -PackageName "spright" -UnzipLocation "$(Split-Path -Parent $MyInvocation.MyCommand.Definition)" -Url64bit "$url64" -ChecksumType "sha256" -Checksum64 $checksum64
