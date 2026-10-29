# Builds web\ages.wasm with wasi-sdk, web\ages-sandbox.html from sandbox.template.html and web\hoard-game.html from game.template.html.
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
$WasiSdk = if ($env:WASI_SDK) { $env:WASI_SDK } else { 'D:\tools\wasi-sdk' }
$Cxx = Join-Path $WasiSdk 'bin\clang++.exe'
$Sysroot = Join-Path $WasiSdk 'share\wasi-sysroot'

$Sources = @(Get-ChildItem sim\core\*.cpp, sim\ages\*.cpp, sim\ages\rules\*.cpp, sim\text\*.cpp | Where-Object { $_.Name -ne 'data_io.cpp' -and $_.Name -ne 'bundle_io.cpp' } | ForEach-Object { $_.FullName })
$Sources += (Resolve-Path sim\wasm\api.cpp).Path
$Sources += (Resolve-Path sim\wasm\text_api.cpp).Path

& $Cxx --target=wasm32-wasi --sysroot=$Sysroot -std=c++17 -O2 -ffp-contract=off -fno-fast-math `
  -fno-exceptions -DJSON_NOEXCEPTION -mexec-model=reactor `
  -Isim `
  -Wl,--export=ages_cmd -Wl,--export=ages_alloc -Wl,--export=ages_free `
  -Wl,--strip-all `
  $Sources -o web\ages.wasm
if ($LASTEXITCODE -ne 0) { throw "clang++ failed" }

$Wasm = [Convert]::ToBase64String([IO.File]::ReadAllBytes('web\ages.wasm'))
$Data = @{}
foreach ($k in 'constants', 'races', 'reactions', 'regions', 'factions', 'shocks', 'names', 'offers') {
    $Data[$k] = Get-Content -Raw "data\$k.json"
}
foreach ($k in 'talents', 'defences') { $Data[$k] = Get-Content -Raw "data\$k.json" }   # v0.4 (S1)
$Data['hooks'] = Get-Content -Raw 'data\hooks.json'   # v0.4 hooks (S3)
$Data['arrivals'] = Get-Content -Raw 'data\arrivals.json'   # item 06
$Data['moves'] = Get-Content -Raw 'data\moves.json'   # item 07
$Data['text'] = Get-Content -Raw 'data\text\en\narrator.json'
$Data['importance'] = Get-Content -Raw 'data\importance.json'
# item 19: one bundle per language as sim/text/bundle_io.cpp merges it
$Bundles = @{}
foreach ($d in Get-ChildItem data\text -Directory | Sort-Object Name) {
    $Parts = @("`"lang`":`"$($d.Name)`"")
    foreach ($f in Get-ChildItem "$($d.FullName)\*.json" | Sort-Object Name) { $Parts += "`"$($f.BaseName)`":" + (Get-Content -Raw -Encoding UTF8 $f.FullName) }
    $Bundles[$d.Name] = '{' + ($Parts -join ',') + '}'
}
$Data['text_bundles'] = $Bundles
# the event cards, joined in file-name order into one JSON array as data_io.cpp does
$Cards = Get-ChildItem data\events\*.json | Sort-Object Name | ForEach-Object { Get-Content -Raw $_.FullName }
$Data['events'] = '[' + ($Cards -join ",`n") + ']'
$Data['genesis'] = Get-Content -Raw 'data\genesis.json'   # v0.4 world (S2)
$DataJson = $Data | ConvertTo-Json -Compress -Depth 2
Write-Host "web\ages.wasm $((Get-Item web\ages.wasm).Length) bytes"
foreach ($pair in @(@('web\sandbox.template.html', 'web\ages-sandbox.html'), @('web\game.template.html', 'web\hoard-game.html'))) {
    $Html = Get-Content -Raw $pair[0]
    $Html = $Html.Replace('%%WASM_BASE64%%', $Wasm).Replace('%%DATA_JSON%%', $DataJson)
    $Html = $Html.Replace('%%THREE_JS%%', (Get-Content -Raw 'web/vendor/three.r158.min.js').Replace('</script', '<\/script'))
    [IO.File]::WriteAllText($pair[1], $Html, [Text.UTF8Encoding]::new($false))
    Write-Host "$($pair[1]) $($Html.Length) chars"
}

python web/inline.py web/hoard-game.html
