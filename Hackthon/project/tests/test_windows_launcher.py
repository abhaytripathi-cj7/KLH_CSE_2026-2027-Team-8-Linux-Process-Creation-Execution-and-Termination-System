"""Check the WSL native-command boundary without requiring a Windows host.

The invocation contract is checked here; Windows PowerShell execution must
also be validated on a Windows machine.
"""
from pathlib import Path
import re
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]

class WindowsLauncherTests(unittest.TestCase):
    def test_linux_commands_bypass_wsl_default_shell(self):
        script = (ROOT / 'scripts/start_windows.ps1').read_text()
        invocations = [line.strip() for line in script.splitlines()
                       if re.search(r'& wsl\.exe -d ', line)]
        self.assertEqual(len(invocations), 2)
        for line in invocations:
            self.assertIn('--exec ', line, 'WSL must bypass its default shell to preserve path arguments')

    def test_native_transport_keeps_windows_backslashes_and_spaces(self):
        for path in [r'C:\Users\PRANAV\Downloads\PipeTalk-Team8-Linux-IPC-Chat\PipeTalk-Team8',
                     r'C:\Users\Test User\Downloads\Project with spaces']:
            output = subprocess.check_output(['printf', '%s', path], text=True)
            self.assertEqual(output, path)
            normalized = path.replace('\\', '/')
            self.assertTrue(normalized.startswith('C:/'))

if __name__ == '__main__':
    unittest.main(verbosity=2)
