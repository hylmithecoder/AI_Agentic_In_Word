$currentPath = Get-Location

if($args[0] -eq "Clean") {
    Remove-Item "$currentPath\AgenticAIOnWord\Debug" -Recurse -Force
    Remove-Item "$currentPath\AgenticAIOnWord\Release" -Recurse -Force
    exit 0
}

if ($args[0] -eq "Debug") {
    $configuration = "Debug"
} else {
    $configuration = "Release"
}

if ($args[1] -eq "isX64") {
    $platform = "x64"
} else {
    $platform = "Win32"
}

msbuild AgenticAIOnWord.vcxproj /p:Configuration=$configuration /p:Platform=$platform /t:Build

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed with exit code $LASTEXITCODE"
    exit $LASTEXITCODE
} else {
    regsvr32 "$currentPath\$configuration\AgenticAIOnWord.dll"
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Registration failed with exit code $LASTEXITCODE"
        exit $LASTEXITCODE
    } else {
        Write-Output "Registration successful"
    }
}