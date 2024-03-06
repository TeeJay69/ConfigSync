$i = 0;

while($i -ne 256){
    Write-Host("#define ANSI_COLOR_$i ""\x1b[38;5;$i" + "m""")
    $i++
}

$i = 1
while($i -ne 256){
    Write-Host
}