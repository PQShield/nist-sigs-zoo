#!/usr/bin/env bash
# Runs inside the container via `scripts/ngcc-run --self-test` (after the
# entry.sh workspace copy, cwd /work). The host side checks the exports.
fail=0
check() { if [ "$2" = 0 ]; then echo "PASS  $1"; else echo "FAIL  $1"; fail=1; fi; }
if getent hosts example.org >/dev/null 2>&1; then r=1; else r=0; fi
check "no DNS (getent hosts example.org fails)" $r
if curl -sS -m 5 -o /dev/null https://example.org 2>/dev/null; then r=1; else r=0; fi
check "no network (curl fails)" $r
n=$(ls /sys/class/net | grep -vc "^lo$" || true)
check "only loopback interface" "$([ "$n" = 0 ] && echo 0 || echo 1)"
if touch /repo/.selftest 2>/dev/null; then rm -f /repo/.selftest; r=1; else r=0; fi
check "/repo is read-only" $r
if touch /repo/bench/results/.selftest 2>/dev/null; then rm -f /repo/bench/results/.selftest; r=1; else r=0; fi
check "/repo/bench/results is read-only" $r
if touch /.selftest 2>/dev/null; then rm -f /.selftest; r=1; else r=0; fi
check "root filesystem is read-only" $r
if touch /work/.selftest 2>/dev/null; then rm -f /work/.selftest; r=0; else r=1; fi
check "/work is writable" $r
check "non-root (uid $(id -u))" "$([ "$(id -u)" != 0 ] && echo 0 || echo 1)"
v=$(gcc -dumpversion | cut -d. -f1)
echo "      gcc major: $v"
check "gcc >= 14" "$([ "$v" -ge 14 ] 2>/dev/null && echo 0 || echo 1)"
# Export probes: the host verifies what reaches .ngcc-out.
mkdir -p bench/results bench/schemes/ngcc/build
echo ok >bench/results/selftest.txt && chmod 755 bench/results/selftest.txt
echo ok >bench/schemes/ngcc/build/selftest.so
echo ok >bench/schemes/ngcc/build/selftest.mk
cp /bin/true bench/results/selftest-elf.log
ln -s /repo/bench bench/results/selftest-link.txt
exit $fail
