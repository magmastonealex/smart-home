mkdir -p wximgs

for i in $(seq -f "%02g" 0 99); do
    echo "$i"
    curl -s -o "wximgs/$i.gif" "https://weather.gc.ca/weathericons/$i.gif"
    file "wximgs/$i.gif" | grep "HTML"
    if [[ $? -eq 0 ]]; then
        rm "wximgs/$i.gif"
    fi
done


