if (Test-Path "$env:ProgramFiles\Git\bin") {
    $env:Path = "$env:ProgramFiles\Git\bin;$env:ProgramFiles\Git\mingw64\bin" + $env:Path
}
elseif (Test-Path "$env:ProgramFiles(x86)\Git\bin") {
    $env:Path = "$env:ProgramFiles(x86)\Git\bin;$env:ProgramFiles(x86)\Git\mingw64\bin;" + $env:Path
}

try {
    sh.exe build.sh $args
}
catch {
    echo "bash.exe doesn't found"
    echo "`r`nInstall Git Bash from:"
    echo "`r`n  https://gitforwindows.org/`r`n"
}
