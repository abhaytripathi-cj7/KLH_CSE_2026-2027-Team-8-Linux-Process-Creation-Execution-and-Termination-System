"""Integration checks using actual Linux children, signals and /proc."""
import re
import subprocess


def run(*args, input=None):
    p = subprocess.run(["./payment", *args], input=input, text=True,
                       capture_output=True, timeout=10)
    assert p.returncode == 0, (p.returncode, p.stdout, p.stderr)
    return p.stdout


for scenario, expected in [
    ("success", "WIFEXITED=true | WEXITSTATUS=0"),
    ("wrong-otp", "WIFEXITED=true | WEXITSTATUS=1"),
    ("decline", "WIFSIGNALED=true | WTERMSIG=15"),
]:
    out = run("--demo", scenario)
    assert expected in out, out
    created = re.search(r"CREATION:.*?\nPid:\s+(\d+).*?\nPPid:\s+(\d+)", out, re.S)
    executed = re.search(r"EXECUTION:.*?\nPid:\s+(\d+)", out, re.S)
    assert created and executed and created[1] == executed[1], out
    assert created[1] != created[2], out
    assert re.search(r"OTP WAIT:.*?State:\s+S", out, re.S), out
    assert re.search(r"TERMINATED:.*?State:\s+Z", out, re.S), out
    assert f"waitpid() returned PID={created[1]}" in out, out
    print(f"PASS: {scenario}, PID/PPID, exec PID continuity, sleeping, zombie, reaping")

for mode in ("1", "2", "3"):
    assert "WEXITSTATUS=0" in run(input=f"{mode}\n250\npay\n123456\n")
print("PASS: interactive UPI, Card and Net banking")
assert "WTERMSIG=15" in run(input="1\n250\npay\ndecline\n")
assert "WTERMSIG=15" in run(input="1\n250\npay\n")
print("PASS: interactive decline and input EOF cleanup")
for value in ("0", "-2", "abc", "1000001", "9" * 100):
    p = subprocess.run(["./payment"], input=f"1\n{value}\n", text=True,
                       capture_output=True, timeout=10)
    assert p.returncode == 2 and "CREATION:" not in p.stdout
print("PASS: invalid amounts do not create payment children")
