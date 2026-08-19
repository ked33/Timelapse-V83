[CmdletBinding()]
param(
    [string]$ResourceScriptPath,
    [string]$ChineseNamesPath,
    [string]$OutputPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$projectDirectory = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($ResourceScriptPath)) {
    $ResourceScriptPath = Join-Path $projectDirectory 'Timelapse.rc'
}
if ([string]::IsNullOrWhiteSpace($ChineseNamesPath)) {
    $ChineseNamesPath = Join-Path $projectDirectory 'Resources\MapNames.zh-CN.tsv'
}
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $projectDirectory 'Generated\MapsList.zh-CN.txt'
}

$resourceScriptFullPath = [System.IO.Path]::GetFullPath($ResourceScriptPath)
$chineseNamesFullPath = [System.IO.Path]::GetFullPath($ChineseNamesPath)
$outputFullPath = [System.IO.Path]::GetFullPath($OutputPath)

foreach ($requiredPath in @($resourceScriptFullPath, $chineseNamesFullPath)) {
    if (-not [System.IO.File]::Exists($requiredPath)) {
        throw "Required map data file was not found: $requiredPath"
    }
}

# Timelapse.rc is UTF-16 LE. Its legacy MapsList resource stores the original
# English route graph as little-endian 16-bit hexadecimal words containing
# UTF-8/ASCII bytes. Decode that graph without changing the original resource.
$resourceText = [System.IO.File]::ReadAllText($resourceScriptFullPath, [System.Text.Encoding]::Unicode)
$resourcePattern = '(?ms)^MapsList\s+TEXT\s*\r?\nBEGIN\s*\r?\n(?<body>.*?)^\s*END\s*$'
$resourceMatch = [System.Text.RegularExpressions.Regex]::Match($resourceText, $resourcePattern)
if (-not $resourceMatch.Success) {
    throw "Could not locate the MapsList TEXT resource in $resourceScriptFullPath."
}

$hexMatches = [System.Text.RegularExpressions.Regex]::Matches(
    $resourceMatch.Groups['body'].Value,
    '0x(?<value>[0-9A-Fa-f]{4})')
if ($hexMatches.Count -eq 0) {
    throw "The MapsList TEXT resource in $resourceScriptFullPath contains no hexadecimal data."
}

$resourceBytes = New-Object byte[] ($hexMatches.Count * 2)
for ($index = 0; $index -lt $hexMatches.Count; $index++) {
    $word = [Convert]::ToUInt16($hexMatches[$index].Groups['value'].Value, 16)
    $resourceBytes[$index * 2] = [byte]($word -band 0xFF)
    $resourceBytes[$index * 2 + 1] = [byte](($word -shr 8) -band 0xFF)
}

$strictUtf8 = New-Object System.Text.UTF8Encoding($false, $true)
$englishMapData = $strictUtf8.GetString($resourceBytes).TrimEnd([char]0)
if (-not $englishMapData.StartsWith('[0]')) {
    throw 'The decoded English MapsList data has an unexpected format.'
}

$chineseNames = @{}
foreach ($rawLine in [System.IO.File]::ReadAllLines($chineseNamesFullPath, $strictUtf8)) {
    if ([string]::IsNullOrWhiteSpace($rawLine) -or $rawLine.StartsWith('#')) {
        continue
    }

    $firstTab = $rawLine.IndexOf("`t")
    $secondTab = if ($firstTab -ge 0) { $rawLine.IndexOf("`t", $firstTab + 1) } else { -1 }
    if ($firstTab -le 0 -or $secondTab -le $firstTab) {
        throw "Invalid Chinese map name row: $rawLine"
    }

    $mapId = 0
    $mapIdText = $rawLine.Substring(0, $firstTab)
    if (-not [int]::TryParse(
            $mapIdText,
            [System.Globalization.NumberStyles]::None,
            [System.Globalization.CultureInfo]::InvariantCulture,
            [ref]$mapId)) {
        throw "Invalid map ID in Chinese map name row: $rawLine"
    }

    $streetName = $rawLine.Substring($firstTab + 1, $secondTab - $firstTab - 1)
    $mapName = $rawLine.Substring($secondTab + 1)
    if ([string]::IsNullOrWhiteSpace($mapName)) {
        throw "Chinese map name is empty for map $mapId."
    }
    if ($streetName.IndexOfAny([char[]]@("`r", "`n", "`t")) -ge 0 -or
        $mapName.IndexOfAny([char[]]@("`r", "`n", "`t")) -ge 0) {
        throw "Chinese map name row contains an unsupported control character for map $mapId."
    }
    if ($chineseNames.ContainsKey($mapId)) {
        throw "Duplicate map ID $mapId in $chineseNamesFullPath."
    }

    $chineseNames[$mapId] = [pscustomobject]@{
        StreetName = $streetName
        MapName = $mapName
    }
}

if ($chineseNames.Count -eq 0) {
    throw "No Chinese map names were loaded from $chineseNamesFullPath."
}

$outputLines = New-Object 'System.Collections.Generic.List[string]'
$missingChineseMapIds = New-Object 'System.Collections.Generic.List[int]'
$englishReader = New-Object System.IO.StringReader($englishMapData)
$mapCount = 0

try {
    while ($null -ne ($headerLine = $englishReader.ReadLine())) {
        if ([string]::IsNullOrWhiteSpace($headerLine)) {
            continue
        }

        $headerMatch = [System.Text.RegularExpressions.Regex]::Match($headerLine, '^\[(\d+)\]$')
        if (-not $headerMatch.Success) {
            throw "Unexpected MapsList line: $headerLine"
        }

        $mapId = [int]::Parse($headerMatch.Groups[1].Value, [System.Globalization.CultureInfo]::InvariantCulture)
        $islandLine = $englishReader.ReadLine()
        $streetLine = $englishReader.ReadLine()
        $mapLine = $englishReader.ReadLine()
        $totalLine = $englishReader.ReadLine()

        if ($null -eq $totalLine -or
            -not $islandLine.StartsWith('island=') -or
            -not $streetLine.StartsWith('streetName=') -or
            -not $mapLine.StartsWith('mapName=') -or
            -not $totalLine.StartsWith('total=')) {
            throw "Map $mapId has an unexpected MapsList block format."
        }

        $portalCount = 0
        if (-not [int]::TryParse(
                $totalLine.Substring(6),
                [System.Globalization.NumberStyles]::None,
                [System.Globalization.CultureInfo]::InvariantCulture,
                [ref]$portalCount) -or $portalCount -lt 0) {
            throw "Map $mapId has an invalid portal count."
        }

        if (-not $chineseNames.ContainsKey($mapId)) {
            $missingChineseMapIds.Add($mapId)
            $chineseEntry = [pscustomobject]@{
                StreetName = $streetLine.Substring(11)
                MapName = $mapLine.Substring(8)
            }
        }
        else {
            $chineseEntry = $chineseNames[$mapId]
        }

        $outputLines.Add($headerLine)
        $outputLines.Add($islandLine)
        $outputLines.Add($streetLine)
        $outputLines.Add($mapLine)
        $outputLines.Add("streetNameZh=$($chineseEntry.StreetName)")
        $outputLines.Add("mapNameZh=$($chineseEntry.MapName)")
        $outputLines.Add($totalLine)

        for ($portalIndex = 0; $portalIndex -lt $portalCount; $portalIndex++) {
            $portalLine = $englishReader.ReadLine()
            if ($null -eq $portalLine) {
                throw "Map $mapId ended before all $portalCount portals were read."
            }

            $portalParts = $portalLine.Split(
                [char[]]@(' '),
                [System.StringSplitOptions]::RemoveEmptyEntries)
            if ($portalParts.Count -lt 5) {
                throw "Map $mapId contains an invalid portal row: $portalLine"
            }
            $outputLines.Add($portalLine)
        }

        $mapCount++
    }
}
finally {
    $englishReader.Dispose()
}

if ($mapCount -eq 0) {
    throw 'No English map blocks were decoded from Timelapse.rc.'
}
if ($missingChineseMapIds.Count -gt 0) {
    $preview = [string]::Join(', ', [string[]]@($missingChineseMapIds | Select-Object -First 20))
    throw "$($missingChineseMapIds.Count) MapsList maps have no Chinese name mapping. First IDs: $preview"
}

$outputText = [string]::Join("`n", [string[]]$outputLines) + "`n"
$outputDirectory = [System.IO.Path]::GetDirectoryName($outputFullPath)
[System.IO.Directory]::CreateDirectory($outputDirectory) | Out-Null

$existingText = if ([System.IO.File]::Exists($outputFullPath)) {
    [System.IO.File]::ReadAllText($outputFullPath, $strictUtf8)
}
else {
    $null
}

if ($existingText -ne $outputText) {
    [System.IO.File]::WriteAllText($outputFullPath, $outputText, $strictUtf8)
    Write-Host "Generated merged UTF-8 map resource with $mapCount maps: $outputFullPath"
}
else {
    Write-Host "Merged UTF-8 map resource is up to date ($mapCount maps): $outputFullPath"
}
