$currentPath = Get-Location

msbuild AgenticAIOnWord.vcxproj /p:Configuration=Debug /p:Platform=Win32 /t:Build

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
} else {
    regsvr32 "$currentPath\Debug\AgenticAIOnWord.dll"
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Registration failed with exit code $LASTEXITCODE"
        exit $LASTEXITCODE
    } else {
        Write-Output "Registration successful"
    }
}