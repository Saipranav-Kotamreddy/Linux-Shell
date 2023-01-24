#!/usr/bin/env python3
from subprocess import Popen, PIPE, STDOUT, TimeoutExpired
import difflib

commands = [
    "echo Hello World",
    "exit",
    [
    "pwd", "cd ..",
    "pwd"
    ],
    [
    "echo HELLO WORLD>file",
    "cat file"
    ],
    "echo HELLO WORLD IS ANYBODY THERE | grep HELLO|wc -l",
    "echo hello| ls Filethat_dont_exist",
    "ls 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16",
    "> file",
    "| grep hi",
    "ls |",
    "echo >",
    "echo hax0r > /etc/passwd",
    "echo Hello world > file | cat file",
    "cd 404notfound",
    "windows7",
    [
    "echo mk ultra cia experiments > output",
    "cat output",
    "echo lulz > output",
    "cat output"
    ],
    [
    "echo research:>output",
    "echo Ruby Ridge 1992 idaho >> output",
    "cat output",
    "echo i am stuck >> output",
    "cat output",
    "echo in a neverending cycle of suffering > output",
    "cat output"
    ],
    "echo > file & | grep memez",
    "cat test.py | base64 -w 80 | head -5",
    [
    "sleep 1&",
    "sleep 2"
    ],
    [
    "sleep 1&",
    "exit",
    "sleep 2",
    "exit"
    ],
]

def print_header(header):
    print("=" * 12)
    print(header)
    print("=" * 12)

def run_and_get_output(path, stdin):
    with Popen([path], stdin=PIPE, stdout=PIPE, stderr=STDOUT) as proc:
        try:
            stdout, stderr = proc.communicate((stdin + "\nexit\n").encode(), timeout=5)
            return stdout.decode("utf-8")
        except TimeoutExpired:
            proc.kill()
            return "5 second process run timeout expired.\n"
    
fail_count = 0

for entry in commands:
    if type(entry) is list:
        command = '\n'.join(entry)
    else:
        command = entry

    print("\n*******\n\nRunning test case...\n")
    print("Commands:")
    print(command)
    print()

    output_expected = run_and_get_output("./sshell_ref", command)
    output = run_and_get_output("./sshell", command)

    if output == output_expected:
          print("PASS")
          continue

    fail_count += 1
    print("FAIL\n")
    print_header("Output")
    print(output)
    print_header("Expected Output")
    print(output_expected)
    print_header("Diff")
    diff = difflib.unified_diff(output.split('\n'), output_expected.split('\n'),
                               n=1, lineterm="", fromfile="result", tofile="expected")
    print('\n'.join(diff))

print("\nAll test cases run.")
print(str(fail_count) + " test case(s) failed.")
    

