for f in /sys/firmware/devicetree/base/compatible /proc/device-tree/compatible; do
    	[ -r "$f" ] || continue
    	if tr '\0' '\n' <"$f" | grep -Eiq '^(apple,(arm-platform|t[0-9]+))$'; then
    		exit 0
    	fi
    done
exit 1